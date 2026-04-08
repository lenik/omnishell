#include "JsonRegistry.hpp"

#include "RegistryDocument.hpp"
#include "RegistryKeyUtil.hpp"
#include "RegistryPath.hpp"
#include "RegistryValueIo.hpp"

#include <boost/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

namespace fs = std::filesystem;

namespace os {

namespace {

std::string readWholeFile(const fs::path& path) {
    std::ifstream in(path);
    if (!in.is_open())
        return "";
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

} // namespace

JsonRegistry::JsonRegistry(std::string root_path) {
    fs::path fp(root_path);
    if (fs::is_regular_file(fp) && fp.extension() == ".json") {
        m_rootDir = fp.parent_path().string();
        m_singleJsonFile = fp.filename().string();
    } else {
        m_rootDir = std::move(root_path);
        m_singleJsonFile.clear();
    }
}

std::string JsonRegistry::fileAbsPath(const std::string& jsonFileRel) const {
    return (fs::path(m_rootDir) / fs::path(jsonFileRel)).string();
}

void JsonRegistry::ensureLoaded() const {
    if (m_loaded)
        return;
    const_cast<JsonRegistry*>(this)->loadFromDisk();
    const_cast<JsonRegistry*>(this)->m_loaded = true;
}

void JsonRegistry::collectKeysForJsonFile(const std::string& jsonFileRel,
                                            std::map<std::vector<std::string>, std::string>& out) const {
    out.clear();
    for (const auto& kv : m_data) {
        reg::JsonTreePathResolution r;
        if (!reg::resolveJsonTreePath(kv.first, r))
            continue;
        if (r.jsonFileRel != jsonFileRel)
            continue;
        out[r.objectPath] = kv.second;
    }
}

void JsonRegistry::syncJsonFile(const std::string& jsonFileRel) {
    std::map<std::vector<std::string>, std::string> leaves;
    collectKeysForJsonFile(jsonFileRel, leaves);

    fs::path abs = fs::path(m_rootDir) / fs::path(jsonFileRel);
    std::error_code ec;
    if (leaves.empty()) {
        fs::remove(abs, ec);
        return;
    }

    boost::json::value doc;
    if (leaves.size() == 1 && leaves.begin()->first.empty()) {
        doc = boost::json::value(leaves.begin()->second);
    } else {
        doc = boost::json::object{};
        for (const auto& e : leaves)
            reg::setNestedString(doc, e.first, e.second);
    }

    fs::create_directories(abs.parent_path(), ec);
    std::ofstream out(abs);
    if (!out.is_open())
        return;
    out << reg::serializeJsonPretty(doc);
}

bool JsonRegistry::loadFromDisk() {
    m_data.clear();

    auto ingestFile = [&](const fs::path& absPath, const std::string& jsonRel) {
        std::string body = readWholeFile(absPath);
        if (body.empty())
            return;
        std::string dualBase = jsonRel.substr(0, jsonRel.size() - 5);
        boost::json::value rootVal = reg::parseJsonOrStringBody(body);
        if (reg::isScalarStringFile(rootVal)) {
            m_data[dualBase] = std::string(rootVal.as_string().c_str());
            return;
        }
        if (!rootVal.is_object())
            return;
        std::function<void(const boost::json::object&, const std::string&)> flatten;
        flatten = [&](const boost::json::object& o, const std::string& dotPath) {
            for (const auto& it : o) {
                const std::string k(it.key());
                std::string nextPath = dotPath.empty() ? k : dotPath + "." + k;
                if (it.value().is_object()) {
                    flatten(it.value().as_object(), nextPath);
                } else if (it.value().is_string()) {
                    std::string fullKey = dualBase.empty() ? nextPath : dualBase + "/" + nextPath;
                    m_data[fullKey] = std::string(it.value().as_string().c_str());
                }
            }
        };
        flatten(rootVal.as_object(), "");
    };

    if (!m_singleJsonFile.empty()) {
        fs::path abs = fs::path(m_rootDir) / m_singleJsonFile;
        if (fs::exists(abs))
            ingestFile(abs, m_singleJsonFile);
        return true;
    }

    if (fs::exists(m_rootDir)) {
        std::function<void(const fs::path&, const std::string&)> walk;
        walk = [&](const fs::path& dir, const std::string& relPrefix) {
            std::error_code ec;
            for (const auto& e : fs::directory_iterator(dir, ec)) {
                if (ec)
                    break;
                if (e.is_directory()) {
                    std::string name = e.path().filename().string();
                    std::string next = relPrefix.empty() ? name : relPrefix + "/" + name;
                    walk(e.path(), next);
                } else if (e.path().extension() == ".json") {
                    std::string name = e.path().filename().string();
                    std::string rel = relPrefix.empty() ? name : relPrefix + "/" + name;
                    ingestFile(e.path(), rel);
                }
            }
        };
        walk(m_rootDir, "");
    }

    return true;
}

bool JsonRegistry::load() {
    m_loaded = false;
    m_data.clear();
    loadFromDisk();
    m_loaded = true;
    return true;
}

bool JsonRegistry::writeAllFiles() const {
    std::map<std::string, bool> seen;
    for (const auto& kv : m_data) {
        reg::JsonTreePathResolution r;
        if (!reg::resolveJsonTreePath(kv.first, r))
            continue;
        if (seen.count(r.jsonFileRel))
            continue;
        seen[r.jsonFileRel] = true;
        const_cast<JsonRegistry*>(this)->syncJsonFile(r.jsonFileRel);
    }
    return true;
}

bool JsonRegistry::save() const {
    ensureLoaded();
    return writeAllFiles();
}

std::vector<std::string> JsonRegistry::list(const std::string& node_key, bool full_key) const {
    ensureLoaded();
    return reg::listChildKeys(m_data, node_key, full_key);
}

reg::variant_t JsonRegistry::getVariant(const std::string& key) const {
    ensureLoaded();
    auto it = m_data.find(key);
    if (it == m_data.end())
        return std::nullopt;
    return reg::parseValue(it->second);
}

void JsonRegistry::setVariant(const std::string& key, reg::variant_t value) {
    ensureLoaded();
    reg::JsonTreePathResolution r;
    if (!reg::resolveJsonTreePath(key, r))
        return;

    reg::variant_t old;
    if (auto it = m_data.find(key); it != m_data.end())
        old = reg::parseValue(it->second);

    if (!value.has_value()) {
        if (m_data.erase(key)) {
            syncJsonFile(r.jsonFileRel);
            emitChanged(key, std::nullopt, old);
        }
        return;
    }
    m_data[key] = reg::valueToString(*value);
    syncJsonFile(r.jsonFileRel);
    emitChanged(key, value, old);
}

bool JsonRegistry::has(const std::string& key) const {
    ensureLoaded();
    return m_data.find(key) != m_data.end();
}

bool JsonRegistry::remove(const std::string& key) {
    ensureLoaded();
    reg::JsonTreePathResolution r;
    if (!reg::resolveJsonTreePath(key, r))
        return false;
    auto it = m_data.find(key);
    if (it == m_data.end())
        return false;
    reg::variant_t old = reg::parseValue(it->second);
    m_data.erase(it);
    syncJsonFile(r.jsonFileRel);
    emitChanged(key, std::nullopt, old);
    return true;
}

bool JsonRegistry::delTree(const std::string& key) {
    ensureLoaded();
    std::vector<std::string> keys = reg::keysForDelTree(m_data, key);
    if (keys.empty())
        return false;
    std::vector<std::string> files;
    for (const auto& k : keys) {
        reg::JsonTreePathResolution r;
        if (reg::resolveJsonTreePath(k, r))
            files.push_back(r.jsonFileRel);
    }
    for (const auto& k : keys) {
        auto it = m_data.find(k);
        if (it == m_data.end())
            continue;
        reg::variant_t old = reg::parseValue(it->second);
        m_data.erase(it);
        emitChanged(k, std::nullopt, old);
    }
    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());
    for (const auto& f : files)
        syncJsonFile(f);
    return true;
}

std::map<std::string, std::string> JsonRegistry::snapshotStrings() const {
    ensureLoaded();
    return m_data;
}

} // namespace os

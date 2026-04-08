#include "FsRegistry.hpp"

#include "RegistryDocument.hpp"
#include "RegistryKeyUtil.hpp"
#include "RegistryPath.hpp"
#include "RegistryValueIo.hpp"

#include <boost/json.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <fstream>
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

constexpr char kScalarLeaf[] = "_";

} // namespace

FsRegistry::FsRegistry(std::string root_dir) : m_rootDir(std::move(root_dir)) {}

std::string FsRegistry::fileAbsPath(const reg::FsPathResolution& r) const {
    fs::path p = m_rootDir;
    for (const auto& d : r.dirSegments)
        p /= d;
    p /= r.fileStem + ".json";
    return p.string();
}

void FsRegistry::ensureLoaded() const {
    if (m_loaded)
        return;
    const_cast<FsRegistry*>(this)->loadFromDisk();
    const_cast<FsRegistry*>(this)->m_loaded = true;
}

void FsRegistry::collectKeysForFsFile(const reg::FsPathResolution& sample,
                                      std::map<std::string, std::string>& out) const {
    out.clear();
    for (const auto& kv : m_data) {
        reg::FsPathResolution r;
        if (!reg::resolveFsPath(kv.first, r))
            continue;
        if (r.dirSegments != sample.dirSegments || r.fileStem != sample.fileStem)
            continue;
        out[r.leafKey] = kv.second;
    }
}

void FsRegistry::syncFsFile(const reg::FsPathResolution& sample) {
    std::map<std::string, std::string> leaves;
    collectKeysForFsFile(sample, leaves);

    fs::path abs = fileAbsPath(sample);
    std::error_code ec;
    if (leaves.empty()) {
        fs::remove(abs, ec);
        return;
    }

    if (leaves.size() == 1 && leaves.begin()->first == kScalarLeaf) {
        std::ofstream out(abs);
        if (!out.is_open())
            return;
        fs::create_directories(abs.parent_path(), ec);
        out << '"' << AbstractRegistry::escapeJsonString(leaves.begin()->second) << '"';
        return;
    }

    boost::json::value doc = boost::json::object{};
    for (const auto& e : leaves)
        doc.as_object()[e.first] = boost::json::value(e.second);

    fs::create_directories(abs.parent_path(), ec);
    std::ofstream out(abs);
    if (!out.is_open())
        return;
    out << reg::serializeJsonPretty(doc);
}

bool FsRegistry::loadFromDisk() {
    m_data.clear();
    if (!fs::exists(m_rootDir))
        return true;

    std::function<void(const fs::path&, const std::vector<std::string>&)> walk;
    walk = [&](const fs::path& dir, const std::vector<std::string>& segs) {
        std::error_code ec;
        for (const auto& e : fs::directory_iterator(dir, ec)) {
            if (ec)
                break;
            if (e.is_directory()) {
                std::vector<std::string> next = segs;
                next.push_back(e.path().filename().string());
                walk(e.path(), next);
            } else if (e.path().extension() == ".json") {
                std::string stem = e.path().stem().string();
                std::string body = readWholeFile(e.path());
                boost::json::value rootVal = reg::parseJsonOrStringBody(body);
                if (reg::isScalarStringFile(rootVal)) {
                    std::string k = reg::makeFsKey(segs, stem, kScalarLeaf);
                    m_data[k] = std::string(rootVal.as_string().c_str());
                } else if (rootVal.is_object()) {
                    for (const auto& it : rootVal.as_object()) {
                        if (!it.value().is_string())
                            continue;
                        std::string k = reg::makeFsKey(segs, stem, std::string(it.key()));
                        m_data[k] = std::string(it.value().as_string().c_str());
                    }
                }
            }
        }
    };

    walk(m_rootDir, {});
    return true;
}

bool FsRegistry::load() {
    m_loaded = false;
    m_data.clear();
    loadFromDisk();
    m_loaded = true;
    return true;
}

bool FsRegistry::writeAllFiles() const {
    std::vector<reg::FsPathResolution> groups;
    for (const auto& kv : m_data) {
        reg::FsPathResolution r;
        if (reg::resolveFsPath(kv.first, r))
            groups.push_back(r);
    }
    std::sort(groups.begin(), groups.end(),
              [](const reg::FsPathResolution& a, const reg::FsPathResolution& b) {
                  if (a.dirSegments != b.dirSegments)
                      return a.dirSegments < b.dirSegments;
                  return a.fileStem < b.fileStem;
              });
    groups.erase(std::unique(groups.begin(), groups.end(),
                             [](const reg::FsPathResolution& a, const reg::FsPathResolution& b) {
                                 return a.dirSegments == b.dirSegments && a.fileStem == b.fileStem;
                             }),
                 groups.end());
    for (const auto& r : groups)
        const_cast<FsRegistry*>(this)->syncFsFile(r);
    return true;
}

bool FsRegistry::save() const {
    ensureLoaded();
    return writeAllFiles();
}

std::vector<std::string> FsRegistry::list(const std::string& node_key, bool full_key) const {
    ensureLoaded();
    return reg::listChildKeys(m_data, node_key, full_key);
}

reg::variant_t FsRegistry::getVariant(const std::string& key) const {
    ensureLoaded();
    auto it = m_data.find(key);
    if (it == m_data.end())
        return std::nullopt;
    return reg::parseValue(it->second);
}

void FsRegistry::setVariant(const std::string& key, reg::variant_t value) {
    ensureLoaded();
    reg::FsPathResolution r;
    if (!reg::resolveFsPath(key, r))
        return;

    reg::variant_t old;
    if (auto it = m_data.find(key); it != m_data.end())
        old = reg::parseValue(it->second);

    if (!value.has_value()) {
        if (m_data.erase(key)) {
            syncFsFile(r);
            emitChanged(key, std::nullopt, old);
        }
        return;
    }
    m_data[key] = reg::valueToString(*value);
    syncFsFile(r);
    emitChanged(key, value, old);
}

bool FsRegistry::has(const std::string& key) const {
    ensureLoaded();
    return m_data.find(key) != m_data.end();
}

bool FsRegistry::remove(const std::string& key) {
    ensureLoaded();
    reg::FsPathResolution r;
    if (!reg::resolveFsPath(key, r))
        return false;
    auto it = m_data.find(key);
    if (it == m_data.end())
        return false;
    reg::variant_t old = reg::parseValue(it->second);
    m_data.erase(it);
    syncFsFile(r);
    emitChanged(key, std::nullopt, old);
    return true;
}

bool FsRegistry::delTree(const std::string& key) {
    ensureLoaded();
    std::vector<std::string> keys = reg::keysForDelTree(m_data, key);
    if (keys.empty())
        return false;
    std::vector<reg::FsPathResolution> groups;
    for (const auto& k : keys) {
        reg::FsPathResolution r;
        if (reg::resolveFsPath(k, r))
            groups.push_back(r);
    }
    for (const auto& k : keys) {
        auto it = m_data.find(k);
        if (it == m_data.end())
            continue;
        reg::variant_t old = reg::parseValue(it->second);
        m_data.erase(it);
        emitChanged(k, std::nullopt, old);
    }
    std::sort(groups.begin(), groups.end(),
              [](const reg::FsPathResolution& a, const reg::FsPathResolution& b) {
                  if (a.dirSegments != b.dirSegments)
                      return a.dirSegments < b.dirSegments;
                  return a.fileStem < b.fileStem;
              });
    groups.erase(std::unique(groups.begin(), groups.end(),
                             [](const reg::FsPathResolution& a, const reg::FsPathResolution& b) {
                                 return a.dirSegments == b.dirSegments && a.fileStem == b.fileStem;
                             }),
                 groups.end());
    for (const auto& r : groups)
        syncFsFile(r);
    return true;
}

std::map<std::string, std::string> FsRegistry::snapshotStrings() const {
    ensureLoaded();
    return m_data;
}

} // namespace os

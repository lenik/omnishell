#include "LocalRegistry.hpp"

#include "RegistryDocument.hpp"
#include "RegistryKeyUtil.hpp"
#include "RegistryPath.hpp"
#include "RegistryValueIo.hpp"

#include <wx/app.h>
#include <wx/log.h>

#include <boost/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

namespace fs = std::filesystem;
namespace reg = os::reg;

namespace {

fs::path dualToFsPath(const fs::path& root, const reg::DualPathResolution& r) {
    fs::path p = root;
    for (const auto& d : r.dirSegments)
        p /= d;
    p /= r.fileStem + ".json";
    return p;
}

std::string readWholeFile(const fs::path& path) {
    std::ifstream in(path);
    if (!in.is_open())
        return "";
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

bool isDirEmpty(const fs::path& dir) {
    if (!fs::exists(dir))
        return true;
    std::error_code ec;
    auto it = fs::directory_iterator(dir, ec);
    if (ec)
        return true;
    return it == fs::directory_iterator{};
}

} // namespace

namespace os {

LocalRegistry& LocalRegistry::instance() {
    static LocalRegistry inst;
    return inst;
}

std::string LocalRegistry::getAppName() const {
    if (wxTheApp) {
        return wxTheApp->GetAppName().ToStdString();
    }
    return "omnishell";
}

std::string LocalRegistry::getRegistryRootDir() const {
    const char* home = std::getenv("HOME");
    fs::path base = home ? fs::path(home) : fs::path{};
    fs::path dir = base / ".config" / getAppName() / "registry";
    return dir.string();
}

std::string LocalRegistry::getLegacyRegistryJsonPath() const {
    const char* home = std::getenv("HOME");
    fs::path base = home ? fs::path(home) : fs::path{};
    fs::path dir = base / ".config" / getAppName();
    return (dir / "registry.json").string();
}

void LocalRegistry::collectKeysForGroup(const reg::DualPathResolution& sample,
                                        std::map<std::vector<std::string>, std::string>& out) const {
    out.clear();
    for (const auto& kv : m_data) {
        reg::DualPathResolution r;
        if (!reg::resolveDualPath(kv.first, r))
            continue;
        if (r.dirSegments != sample.dirSegments || r.fileStem != sample.fileStem)
            continue;
        out[r.objectPath] = kv.second;
    }
}

void LocalRegistry::syncDualFileForGroup(const reg::DualPathResolution& sample) {
    fs::path root = getRegistryRootDir();
    std::map<std::vector<std::string>, std::string> leaves;
    collectKeysForGroup(sample, leaves);

    fs::path p = dualToFsPath(root, sample);
    std::error_code ec;

    if (leaves.empty()) {
        fs::remove(p, ec);
        return;
    }

    bool hasNested = false;
    for (const auto& e : leaves) {
        if (!e.first.empty()) {
            hasNested = true;
            break;
        }
    }

    boost::json::value doc;
    if (!hasNested && leaves.size() == 1 && leaves.begin()->first.empty()) {
        doc = boost::json::value(leaves.begin()->second);
    } else {
        doc = boost::json::object{};
        for (const auto& e : leaves) {
            if (!e.first.empty())
                reg::setNestedString(doc, e.first, e.second);
        }
    }

    fs::create_directories(p.parent_path(), ec);
    std::ofstream out(p);
    if (!out.is_open()) {
        wxLogWarning("LocalRegistry: cannot write %s", p.string());
        return;
    }
    out << reg::serializeJsonPretty(doc);
}

void LocalRegistry::ensureLoaded() {
    if (m_loaded)
        return;
    loadFromDiskOrMigrate();
    m_loaded = true;
}

void LocalRegistry::loadFromDiskOrMigrate() {
    m_data.clear();

    fs::path root = getRegistryRootDir();
    std::string legacyPath = getLegacyRegistryJsonPath();

    if (fs::exists(legacyPath) && isDirEmpty(root)) {
        if (parseLegacyFlatJsonFile(legacyPath)) {
            std::error_code ec;
            fs::create_directories(root, ec);
            if (writeAllFiles()) {
                try {
                    fs::rename(fs::path(legacyPath), fs::path(legacyPath + ".bak"));
                } catch (...) {
                }
                wxLogInfo("LocalRegistry: migrated legacy registry.json (%zu entries)", m_data.size());
                return;
            }
            m_data.clear();
            wxLogWarning("LocalRegistry: migration write failed; falling back to scan/seed");
        }
    }

    if (fs::exists(root)) {
        std::function<void(const fs::path&, const std::string&)> walk;
        walk = [&](const fs::path& dir, const std::string& slashPrefix) {
            std::error_code ec;
            for (const auto& e : fs::directory_iterator(dir, ec)) {
                if (ec)
                    break;
                if (e.is_directory()) {
                    std::string seg = e.path().filename().string();
                    std::string next = slashPrefix.empty() ? seg : slashPrefix + "/" + seg;
                    walk(e.path(), next);
                } else if (e.path().extension() == ".json") {
                    std::string stem = e.path().stem().string();
                    std::string dualBase = slashPrefix.empty() ? stem : slashPrefix + "/" + stem;
                    std::string body = readWholeFile(e.path());
                    boost::json::value rootVal = reg::parseJsonOrStringBody(body);
                    if (reg::isScalarStringFile(rootVal)) {
                        m_data[dualBase] = std::string(rootVal.as_string().c_str());
                    } else if (rootVal.is_object()) {
                        std::function<void(const boost::json::object&, const std::string&)> flatten;
                        flatten = [&](const boost::json::object& o, const std::string& dotPath) {
                            for (const auto& it : o) {
                                const std::string k(it.key());
                                std::string nextPath = dotPath.empty() ? k : dotPath + "." + k;
                                if (it.value().is_object()) {
                                    flatten(it.value().as_object(), nextPath);
                                } else if (it.value().is_string()) {
                                    m_data[dualBase + "." + nextPath] =
                                        std::string(it.value().as_string().c_str());
                                }
                            }
                        };
                        flatten(rootVal.as_object(), "");
                    }
                }
            }
        };
        walk(root, "");
    }

    if (m_data.empty()) {
        m_data["System/OS/Name"] = "Omnishell";
        m_data["System/OS/Version"] = "1.1.1";
        m_data["User/Name"] = "Guest";
        m_data["User/Role"] = "Guest";
        std::error_code ec;
        fs::create_directories(root, ec);
        writeAllFiles();
        wxLogInfo("LocalRegistry: seeded defaults (%zu entries)", m_data.size());
    } else {
        wxLogInfo("LocalRegistry: loaded %zu entries from filesystem", m_data.size());
    }
}

bool LocalRegistry::parseLegacyFlatJsonFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open())
        return false;

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string json = buffer.str();

    size_t pos = 0;
    while (true) {
        size_t keyPos = json.find('"', pos);
        if (keyPos == std::string::npos)
            break;
        size_t keyEnd = json.find('"', keyPos + 1);
        if (keyEnd == std::string::npos)
            break;
        std::string key = json.substr(keyPos + 1, keyEnd - keyPos - 1);

        size_t colon = json.find(':', keyEnd);
        if (colon == std::string::npos)
            break;
        size_t valStart = json.find('"', colon);
        if (valStart == std::string::npos)
            break;
        size_t valEnd = json.find('"', valStart + 1);
        if (valEnd == std::string::npos)
            break;
        std::string value = json.substr(valStart + 1, valEnd - valStart - 1);

        m_data[reg::migrateLegacyFlatKey(key)] = value;
        pos = valEnd + 1;
    }

    return !m_data.empty();
}

bool LocalRegistry::load() {
    m_loaded = false;
    m_data.clear();
    loadFromDiskOrMigrate();
    m_loaded = true;
    return true;
}

bool LocalRegistry::writeAllFiles() {
    fs::path root = getRegistryRootDir();
    std::error_code ec;
    fs::create_directories(root, ec);

    std::map<std::pair<std::vector<std::string>, std::string>, bool> seen;
    for (const auto& kv : m_data) {
        reg::DualPathResolution r;
        if (!reg::resolveDualPath(kv.first, r))
            continue;
        auto gk = std::make_pair(r.dirSegments, r.fileStem);
        if (seen.count(gk))
            continue;
        seen[gk] = true;
        syncDualFileForGroup(r);
    }
    return true;
}

bool LocalRegistry::save() const {
    const_cast<LocalRegistry*>(this)->ensureLoaded();
    return const_cast<LocalRegistry*>(this)->writeAllFiles();
}

std::vector<std::string> LocalRegistry::list(const std::string& node_key, bool full_key) const {
    const_cast<LocalRegistry*>(this)->ensureLoaded();
    return reg::listChildKeys(m_data, reg::normalizeDualLookupKey(node_key), full_key);
}

reg::variant_t LocalRegistry::getVariant(const std::string& key) const {
    const_cast<LocalRegistry*>(this)->ensureLoaded();
    auto it = m_data.find(reg::normalizeDualLookupKey(key));
    if (it == m_data.end())
        return std::nullopt;
    return reg::parseValue(it->second);
}

void LocalRegistry::setVariant(const std::string& key, reg::variant_t value) {
    ensureLoaded();
    const std::string nk = reg::normalizeDualLookupKey(key);
    reg::DualPathResolution r;
    if (!reg::resolveDualPath(nk, r)) {
        wxLogWarning("LocalRegistry: invalid key (Dual path): %s", key);
        return;
    }

    reg::variant_t old;
    if (auto it = m_data.find(nk); it != m_data.end())
        old = reg::parseValue(it->second);
    else
        old = std::nullopt;

    if (!value.has_value()) {
        if (m_data.erase(nk)) {
            syncDualFileForGroup(r);
            emitChanged(nk, std::nullopt, old);
        }
        return;
    }

    m_data[nk] = reg::valueToString(*value);
    syncDualFileForGroup(r);
    emitChanged(nk, value, old);
}

bool LocalRegistry::has(const std::string& key) const {
    const_cast<LocalRegistry*>(this)->ensureLoaded();
    return m_data.find(reg::normalizeDualLookupKey(key)) != m_data.end();
}

bool LocalRegistry::remove(const std::string& key) {
    ensureLoaded();
    const std::string nk = reg::normalizeDualLookupKey(key);
    reg::DualPathResolution r;
    if (!reg::resolveDualPath(nk, r))
        return false;
    auto it = m_data.find(nk);
    if (it == m_data.end())
        return false;
    reg::variant_t old = reg::parseValue(it->second);
    m_data.erase(it);
    syncDualFileForGroup(r);
    emitChanged(nk, std::nullopt, old);
    return true;
}

bool LocalRegistry::delTree(const std::string& key) {
    ensureLoaded();
    std::vector<std::string> keys =
        reg::keysForDelTree(m_data, reg::normalizeDualLookupKey(key));
    if (keys.empty())
        return false;
    std::vector<reg::DualPathResolution> groups;
    for (const auto& k : keys) {
        reg::DualPathResolution r;
        if (reg::resolveDualPath(k, r))
            groups.push_back(std::move(r));
    }
    for (const auto& k : keys) {
        auto it = m_data.find(k);
        if (it == m_data.end())
            continue;
        reg::variant_t old = reg::parseValue(it->second);
        m_data.erase(it);
        emitChanged(k, std::nullopt, old);
    }
    auto uniq = [](std::vector<reg::DualPathResolution>& g) {
        std::sort(g.begin(), g.end(),
                  [](const reg::DualPathResolution& a, const reg::DualPathResolution& b) {
                      if (a.dirSegments != b.dirSegments)
                          return a.dirSegments < b.dirSegments;
                      return a.fileStem < b.fileStem;
                  });
        g.erase(std::unique(g.begin(), g.end(),
                            [](const reg::DualPathResolution& a, const reg::DualPathResolution& b) {
                                return a.dirSegments == b.dirSegments && a.fileStem == b.fileStem;
                            }),
                g.end());
    };
    uniq(groups);
    for (const auto& r : groups)
        syncDualFileForGroup(r);
    return true;
}

std::map<std::string, std::string> LocalRegistry::snapshotStrings() const {
    const_cast<LocalRegistry*>(this)->ensureLoaded();
    return m_data;
}

} // namespace os

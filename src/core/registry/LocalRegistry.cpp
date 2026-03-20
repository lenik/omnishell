#include "LocalRegistry.hpp"

#include "RegistryKeyUtil.hpp"
#include "RegistryValueIo.hpp"

#include <wx/app.h>
#include <wx/log.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

namespace fs = std::filesystem;

namespace {

std::vector<std::string> splitKey(const std::string& s) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start < s.size()) {
        size_t dot = s.find('.', start);
        if (dot == std::string::npos) {
            out.push_back(s.substr(start));
            break;
        }
        out.push_back(s.substr(start, dot - start));
        start = dot + 1;
    }
    return out;
}

std::string escapeJson(const std::string& in) {
    std::string out;
    out.reserve(in.size() + 8);
    for (char c : in) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
        }
    }
    return out;
}

std::string parseJsonStringLiteral(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\n' || s.front() == '\r'))
        s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\n' || s.back() == '\r'))
        s.remove_suffix(1);
    if (s.empty())
        return "";
    if (s.front() != '"')
        return std::string(s);

    std::string out;
    for (size_t i = 1; i < s.size(); ++i) {
        char c = s[i];
        if (c == '"')
            break;
        if (c == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
            case 'n':
                out += '\n';
                break;
            case 'r':
                out += '\r';
                break;
            case 't':
                out += '\t';
                break;
            case '\\':
                out += '\\';
                break;
            case '"':
                out += '"';
                break;
            default:
                out += s[i];
                break;
            }
        } else {
            out += c;
        }
    }
    return out;
}

fs::path keyToFilePath(const fs::path& root, const std::string& key) {
    auto parts = splitKey(key);
    if (parts.empty())
        return root;
    fs::path p = root;
    for (size_t i = 0; i + 1 < parts.size(); ++i)
        p /= parts[i];
    p /= parts.back() + ".json";
    return p;
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

std::string readWholeFile(const fs::path& path) {
    std::ifstream in(path);
    if (!in.is_open())
        return "";
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
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
        walk = [&](const fs::path& dir, const std::string& prefix) {
            std::error_code ec;
            for (const auto& e : fs::directory_iterator(dir, ec)) {
                if (ec)
                    break;
                if (e.is_directory()) {
                    std::string seg = e.path().filename().string();
                    std::string next = prefix.empty() ? seg : prefix + "." + seg;
                    walk(e.path(), next);
                } else if (e.path().extension() == ".json") {
                    std::string stem = e.path().stem().string();
                    std::string key = prefix.empty() ? stem : prefix + "." + stem;
                    m_data[key] = parseJsonStringLiteral(readWholeFile(e.path()));
                }
            }
        };
        walk(root, "");
    }

    if (m_data.empty()) {
        m_data["System.OS.Name"] = "Omnishell";
        m_data["System.OS.Version"] = "1.1.1";
        m_data["User.Name"] = "Guest";
        m_data["User.Role"] = "Guest";
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

        m_data[key] = value;
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

    for (const auto& kv : m_data) {
        fs::path p = keyToFilePath(root, kv.first);
        fs::create_directories(p.parent_path(), ec);
        std::ofstream out(p);
        if (!out.is_open()) {
            wxLogWarning("LocalRegistry: cannot write %s", p.string());
            return false;
        }
        out << '"' << escapeJson(kv.second) << '"';
    }
    return true;
}

bool LocalRegistry::save() const {
    const_cast<LocalRegistry*>(this)->ensureLoaded();
    return const_cast<LocalRegistry*>(this)->writeAllFiles();
}

std::vector<std::string> LocalRegistry::list(const std::string& node_key, bool full_key) const {
    const_cast<LocalRegistry*>(this)->ensureLoaded();
    return reg::listChildKeys(m_data, node_key, full_key);
}

reg::variant_t LocalRegistry::getVariant(const std::string& key) const {
    const_cast<LocalRegistry*>(this)->ensureLoaded();
    auto it = m_data.find(key);
    if (it == m_data.end())
        return std::nullopt;
    return reg::parseValue(it->second);
}

void LocalRegistry::setVariant(const std::string& key, reg::variant_t value) {
    ensureLoaded();
    reg::variant_t old;
    if (auto it = m_data.find(key); it != m_data.end())
        old = reg::parseValue(it->second);
    else
        old = std::nullopt;

    if (!value.has_value()) {
        if (m_data.erase(key)) {
            fs::path p = keyToFilePath(getRegistryRootDir(), key);
            std::error_code ec;
            fs::remove(p, ec);
            emitChanged(key, std::nullopt, old);
        }
        return;
    }

    std::string s = reg::valueToString(*value);
    m_data[key] = std::move(s);
    emitChanged(key, value, old);
}

bool LocalRegistry::has(const std::string& key) const {
    const_cast<LocalRegistry*>(this)->ensureLoaded();
    return m_data.find(key) != m_data.end();
}

bool LocalRegistry::remove(const std::string& key) {
    ensureLoaded();
    auto it = m_data.find(key);
    if (it == m_data.end())
        return false;
    reg::variant_t old = reg::parseValue(it->second);
    m_data.erase(it);
    fs::path p = keyToFilePath(getRegistryRootDir(), key);
    std::error_code ec;
    fs::remove(p, ec);
    emitChanged(key, std::nullopt, old);
    return true;
}

bool LocalRegistry::delTree(const std::string& key) {
    ensureLoaded();
    std::vector<std::string> keys = reg::keysForDelTree(m_data, key);
    if (keys.empty())
        return false;
    fs::path root = getRegistryRootDir();
    for (const auto& k : keys) {
        auto it = m_data.find(k);
        if (it == m_data.end())
            continue;
        reg::variant_t old = reg::parseValue(it->second);
        m_data.erase(it);
        fs::path p = keyToFilePath(root, k);
        std::error_code ec;
        fs::remove(p, ec);
        emitChanged(k, std::nullopt, old);
    }
    return true;
}

std::map<std::string, std::string> LocalRegistry::snapshotStrings() const {
    const_cast<LocalRegistry*>(this)->ensureLoaded();
    return m_data;
}

} // namespace os

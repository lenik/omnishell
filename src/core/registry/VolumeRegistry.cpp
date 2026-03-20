#include "VolumeRegistry.hpp"

#include "RegistryKeyUtil.hpp"
#include "RegistryValueIo.hpp"

#include <wx/log.h>

#include <algorithm>
#include <cctype>

namespace os {

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

std::string keyToRelPath(const std::string& key) {
    auto parts = splitKey(key);
    if (parts.empty())
        return "";
    std::string rel;
    for (size_t i = 0; i + 1 < parts.size(); ++i)
        rel += parts[i] + "/";
    rel += parts.back() + ".json";
    return rel;
}

std::string escapeJson(const std::string& in) {
    std::string out;
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
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
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

bool endsWithJson(std::string_view n) {
    return n.size() >= 5 && n.compare(n.size() - 5, 5, ".json") == 0;
}

} // namespace

VolumeRegistry::VolumeRegistry(VolumeFile root) : m_root(std::move(root)) {}

void VolumeRegistry::ensureLoaded() const {
    if (m_loaded)
        return;
    if (!m_root.isNotEmpty()) {
        const_cast<VolumeRegistry*>(this)->m_loaded = true;
        return;
    }
    const_cast<VolumeRegistry*>(this)->loadFromVolume();
    const_cast<VolumeRegistry*>(this)->m_loaded = true;
}

void VolumeRegistry::loadFromVolume() {
    m_data.clear();
    if (!m_root.isNotEmpty())
        return;

    std::function<void(const VolumeFile&, const std::string&)> walk;
    walk = [&](const VolumeFile& dir, const std::string& prefix) {
        std::vector<std::unique_ptr<FileStatus>> list;
        dir.readDir(list, false);
        for (const auto& es : list) {
            if (!es)
                continue;
            if (es->isDirectory()) {
                auto sub = dir.resolve(es->name);
                if (sub && sub->isNotEmpty()) {
                    std::string next =
                        prefix.empty() ? std::string(es->name) : prefix + "." + std::string(es->name);
                    walk(*sub, next);
                }
            } else if (endsWithJson(es->name)) {
                std::string name = es->name;
                std::string stem = name.substr(0, name.size() - 5);
                std::string key = prefix.empty() ? stem : prefix + "." + stem;
                auto f = dir.resolve(es->name);
                if (f && f->isNotEmpty() && f->isFile()) {
                    try {
                        m_data[key] = parseJsonStringLiteral(f->readFileString("UTF-8"));
                    } catch (...) {
                    }
                }
            }
        }
    };

    try {
        walk(m_root, "");
    } catch (...) {
        wxLogWarning("VolumeRegistry: load walk failed");
    }

    if (m_data.empty() && m_root.isNotEmpty()) {
        m_data["System.OS.Name"] = "Omnishell";
        m_data["System.OS.Version"] = "1.1.1";
        m_data["User.Name"] = "Guest";
        m_data["User.Role"] = "Guest";
        if (!writeAllToVolume())
            wxLogWarning("VolumeRegistry: seed write failed");
        else
            wxLogInfo("VolumeRegistry: seeded defaults (%zu entries)", m_data.size());
    } else if (!m_data.empty()) {
        wxLogInfo("VolumeRegistry: loaded %zu entries from volume", m_data.size());
    }
}

std::unique_ptr<VolumeFile> VolumeRegistry::fileForKey(const std::string& key) const {
    if (!m_root.isNotEmpty())
        return nullptr;
    std::string rel = keyToRelPath(key);
    if (rel.empty())
        return nullptr;
    return m_root.resolve(rel);
}

bool VolumeRegistry::writeAllToVolume() const {
    if (!m_root.isNotEmpty())
        return false;
    for (const auto& kv : m_data) {
        auto f = fileForKey(kv.first);
        if (!f || !f->isNotEmpty())
            return false;
        if (!f->createParentDirectories())
            return false;
        std::string body = "\"" + escapeJson(kv.second) + "\"";
        try {
            f->writeFileString(body, "UTF-8");
        } catch (...) {
            return false;
        }
    }
    return true;
}

bool VolumeRegistry::load() {
    m_loaded = false;
    m_data.clear();
    loadFromVolume();
    m_loaded = true;
    return true;
}

bool VolumeRegistry::save() const {
    ensureLoaded();
    return writeAllToVolume();
}

std::vector<std::string> VolumeRegistry::list(const std::string& node_key, bool full_key) const {
    ensureLoaded();
    return reg::listChildKeys(m_data, node_key, full_key);
}

reg::variant_t VolumeRegistry::getVariant(const std::string& key) const {
    ensureLoaded();
    auto it = m_data.find(key);
    if (it == m_data.end())
        return std::nullopt;
    return reg::parseValue(it->second);
}

void VolumeRegistry::setVariant(const std::string& key, reg::variant_t value) {
    ensureLoaded();
    reg::variant_t old;
    if (auto it = m_data.find(key); it != m_data.end())
        old = reg::parseValue(it->second);

    if (!value.has_value()) {
        if (m_data.erase(key)) {
            auto f = fileForKey(key);
            if (f && f->isNotEmpty())
                f->removeFile();
            emitChanged(key, std::nullopt, old);
        }
        return;
    }
    m_data[key] = reg::valueToString(*value);
    emitChanged(key, value, old);
}

bool VolumeRegistry::has(const std::string& key) const {
    ensureLoaded();
    return m_data.find(key) != m_data.end();
}

bool VolumeRegistry::remove(const std::string& key) {
    ensureLoaded();
    auto it = m_data.find(key);
    if (it == m_data.end())
        return false;
    reg::variant_t oldv = reg::parseValue(it->second);
    m_data.erase(it);
    auto f = fileForKey(key);
    if (f && f->isNotEmpty())
        f->removeFile();
    emitChanged(key, std::nullopt, oldv);
    return true;
}

bool VolumeRegistry::delTree(const std::string& key) {
    ensureLoaded();
    std::vector<std::string> keys = reg::keysForDelTree(m_data, key);
    if (keys.empty())
        return false;
    for (const auto& k : keys) {
        auto it = m_data.find(k);
        if (it == m_data.end())
            continue;
        reg::variant_t oldv = reg::parseValue(it->second);
        m_data.erase(it);
        auto f = fileForKey(k);
        if (f && f->isNotEmpty())
            f->removeFile();
        emitChanged(k, std::nullopt, oldv);
    }
    return true;
}

std::map<std::string, std::string> VolumeRegistry::snapshotStrings() const {
    ensureLoaded();
    return m_data;
}

} // namespace os

#include "JsonRegistry.hpp"

#include "RegistryKeyUtil.hpp"
#include "RegistryValueIo.hpp"

#include <fstream>
#include <sstream>

namespace os {

namespace {

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

} // namespace

JsonRegistry::JsonRegistry(std::string json_file_path) : m_path(std::move(json_file_path)) {}

void JsonRegistry::ensureLoaded() const {
    if (m_loaded)
        return;
    std::ifstream in(m_path);
    if (!in.is_open()) {
        const_cast<JsonRegistry*>(this)->m_loaded = true;
        return;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    const_cast<JsonRegistry*>(this)->parseFlatJson(buffer.str());
    const_cast<JsonRegistry*>(this)->m_loaded = true;
}

bool JsonRegistry::parseFlatJson(const std::string& json) {
    m_data.clear();
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
    return true;
}

bool JsonRegistry::load() {
    m_loaded = false;
    m_data.clear();
    std::ifstream in(m_path);
    if (!in.is_open()) {
        m_loaded = true;
        return true;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    parseFlatJson(buffer.str());
    m_loaded = true;
    return true;
}

bool JsonRegistry::writeFlatJson() const {
    std::ofstream out(m_path);
    if (!out.is_open())
        return false;
    out << "{\n";
    size_t i = 0;
    for (const auto& kv : m_data) {
        out << "  \"" << escapeJson(kv.first) << "\": \"" << escapeJson(kv.second) << "\"";
        if (i + 1 < m_data.size())
            out << ",";
        out << "\n";
        ++i;
    }
    out << "}\n";
    return true;
}

bool JsonRegistry::save() const {
    ensureLoaded();
    return writeFlatJson();
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
    reg::variant_t old;
    if (auto it = m_data.find(key); it != m_data.end())
        old = reg::parseValue(it->second);

    if (!value.has_value()) {
        if (m_data.erase(key))
            emitChanged(key, std::nullopt, old);
        return;
    }
    m_data[key] = reg::valueToString(*value);
    emitChanged(key, value, old);
}

bool JsonRegistry::has(const std::string& key) const {
    ensureLoaded();
    return m_data.find(key) != m_data.end();
}

bool JsonRegistry::remove(const std::string& key) {
    ensureLoaded();
    auto it = m_data.find(key);
    if (it == m_data.end())
        return false;
    reg::variant_t old = reg::parseValue(it->second);
    m_data.erase(it);
    emitChanged(key, std::nullopt, old);
    return true;
}

bool JsonRegistry::delTree(const std::string& key) {
    ensureLoaded();
    std::vector<std::string> keys = reg::keysForDelTree(m_data, key);
    if (keys.empty())
        return false;
    for (const auto& k : keys) {
        auto it = m_data.find(k);
        if (it == m_data.end())
            continue;
        reg::variant_t old = reg::parseValue(it->second);
        m_data.erase(it);
        emitChanged(k, std::nullopt, old);
    }
    return true;
}

std::map<std::string, std::string> JsonRegistry::snapshotStrings() const {
    ensureLoaded();
    return m_data;
}

} // namespace os

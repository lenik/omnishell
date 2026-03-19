#include "RegistryDb.hpp"

#include <wx/app.h>
#include <wx/log.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

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

} // namespace

namespace os {

RegistryDb& RegistryDb::getInstance() {
    static RegistryDb instance;
    return instance;
}

RegistryDb::RegistryDb() = default;

std::string RegistryDb::getAppName() const {
    if (wxTheApp) {
        return wxTheApp->GetAppName().ToStdString();
    }
    return "omnishell";
}

std::string RegistryDb::getConfigPath() const {
    namespace fs = std::filesystem;
    const char* home = std::getenv("HOME");
    fs::path base = home ? fs::path(home) : fs::path{};
    fs::path dir = base / ".config" / getAppName();
    std::error_code ec;
    fs::create_directories(dir, ec);
    fs::path file = dir / "registry.json";
    return file.string();
}

bool RegistryDb::load() {
    data_.clear();

    std::string path = getConfigPath();
    std::ifstream in(path);
    if (!in.is_open()) {
        // No registry yet - create a default JSON structure, then proceed with loading.
        std::ofstream out(path);
        if (out.is_open()) {
            out << "{\n"
                   "  \"System.OS.Name\": \"Omnishell\",\n"
                   "  \"System.OS.Version\": \"1.1.1\",\n"
                   "  \"User.Name\": \"Guest\",\n"
                   "  \"User.Role\": \"Guest\"\n"
                   "}\n";
            out.close();
        }
        in.open(path);
        if (!in.is_open()) {
            // If we still cannot open, just start with empty data.
            return true;
        }
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string json = buffer.str();

    // Very small JSON parser for flat string map: { "key": "value", ... }
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

        data_[key] = value;
        pos = valEnd + 1;
    }

    wxLogInfo("RegistryDb loaded %zu entries", data_.size());
    return true;
}

bool RegistryDb::save() const {
    std::string path = getConfigPath();
    std::ofstream out(path);
    if (!out.is_open()) {
        wxLogWarning("RegistryDb: cannot open %s for writing", path);
        return false;
    }

    out << "{\n";
    size_t i = 0;
    for (const auto& kv : data_) {
        out << "  \"" << escapeJson(kv.first) << "\": \"" << escapeJson(kv.second) << "\"";
        if (i + 1 < data_.size())
            out << ",";
        out << "\n";
        ++i;
    }
    out << "}\n";
    return true;
}

void RegistryDb::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

std::string RegistryDb::get(const std::string& key, const std::string& defaultValue) const {
    auto it = data_.find(key);
    if (it == data_.end())
        return defaultValue;
    return it->second;
}

bool RegistryDb::has(const std::string& key) const {
    return data_.find(key) != data_.end();
}

void RegistryDb::remove(const std::string& key) {
    data_.erase(key);
}

} // namespace os


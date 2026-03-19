#ifndef OMNISHELL_CORE_REGISTRY_DB_HPP
#define OMNISHELL_CORE_REGISTRY_DB_HPP

#include <map>
#include <string>

namespace os {

/**
 * RegistryDb
 *
 * Lightweight JSON-backed key/value store.
 * Stored at: ~/.config/<appname>/registry.json
 */
class RegistryDb {
public:
    static RegistryDb& getInstance();

    bool load();
    bool save() const;

    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key, const std::string& defaultValue = "") const;
    bool has(const std::string& key) const;
    void remove(const std::string& key);

    const std::map<std::string, std::string>& data() const { return data_; }

private:
    RegistryDb();

    std::string getAppName() const;
    std::string getConfigPath() const;

    std::map<std::string, std::string> data_;
};

} // namespace os

#endif // OMNISHELL_CORE_REGISTRY_DB_HPP


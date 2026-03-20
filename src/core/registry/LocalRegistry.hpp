#ifndef OMNISHELL_CORE_LOCAL_REGISTRY_HPP
#define OMNISHELL_CORE_LOCAL_REGISTRY_HPP

#include "IRegistry.hpp"

#include <map>
#include <string>

namespace os {

/**
 * Registry stored as ~/.config/<app>/registry/ tree: dirs = nodes, leaf = .json string value.
 */
class LocalRegistry : public IRegistry {
  public:
    static LocalRegistry& instance();

    bool load() override;
    bool save() const override;

    std::vector<std::string> list(const std::string& node_key, bool full_key) const override;

    reg::variant_t getVariant(const std::string& key) const override;
    void setVariant(const std::string& key, reg::variant_t value) override;

    bool has(const std::string& key) const override;
    bool remove(const std::string& key) override;
    bool delTree(const std::string& key) override;

    std::map<std::string, std::string> snapshotStrings() const override;

  private:
    LocalRegistry() = default;

    void ensureLoaded();
    void loadFromDiskOrMigrate();
    bool writeAllFiles();
    bool parseLegacyFlatJsonFile(const std::string& path);

    std::string getAppName() const;
    std::string getRegistryRootDir() const;
    std::string getLegacyRegistryJsonPath() const;

    mutable bool m_loaded{false};
    mutable std::map<std::string, std::string> m_data;
};

} // namespace os

#endif

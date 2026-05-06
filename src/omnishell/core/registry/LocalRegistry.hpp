#ifndef OMNISHELL_CORE_LOCAL_REGISTRY_HPP
#define OMNISHELL_CORE_LOCAL_REGISTRY_HPP

#include "AbstractRegistry.hpp"
#include "RegistryPath.hpp"

#include <map>
#include <string>
#include <vector>

namespace os {

/**
 * Dual layout registry: '/' = directories and JSON filename stem under ~/.config/<app>/registry/;
 * '.' = object path inside that .json file. Scalar files use a single JSON string body.
 */
class LocalRegistry : public AbstractRegistry {
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

    void syncDualFileForGroup(const reg::DualPathResolution& sample);
    void collectKeysForGroup(const reg::DualPathResolution& sample,
                             std::map<std::vector<std::string>, std::string>& out) const;

    std::string getAppName() const;
    std::string getRegistryRootDir() const;
    std::string getLegacyRegistryJsonPath() const;
};

} // namespace os

#endif

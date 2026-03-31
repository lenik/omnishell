#ifndef OMNISHELL_CORE_VOLUME_REGISTRY_HPP
#define OMNISHELL_CORE_VOLUME_REGISTRY_HPP

#include "IRegistry.hpp"

#include <bas/volume/VolumeFile.hpp>

#include <map>
#include <memory>
#include <string>

namespace os {

/**
 * Registry tree on a Volume: root path + segments as dirs, leaf ".json" files (JSON string body).
 */
class VolumeRegistry : public IRegistry {
  public:
    explicit VolumeRegistry(VolumeFile root);

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
    void ensureLoaded() const;

    void loadFromVolume();
    bool writeAllToVolume() const;
    std::unique_ptr<VolumeFile> fileForKey(const std::string& key) const;

    VolumeFile m_root;
    mutable bool m_loaded{false};
    mutable std::map<std::string, std::string> m_data;
};

} // namespace os

#endif

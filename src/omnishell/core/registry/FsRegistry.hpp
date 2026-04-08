#ifndef OMNISHELL_CORE_FS_REGISTRY_HPP
#define OMNISHELL_CORE_FS_REGISTRY_HPP

#include "AbstractRegistry.hpp"

#include <string>

namespace os {

/**
 * Filesystem registry: '/' and '.' are both path segments; the second-to-last segment is the JSON
 * filename stem, the last is the property name inside that file.
 * Example: foo/bar/cat.dog.k -> <root>/foo/bar/cat/dog.json { "k": "..." }.
 */
class FsRegistry : public AbstractRegistry {
  public:
    explicit FsRegistry(std::string root_dir);

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

    bool loadFromDisk();
    bool writeAllFiles() const;

    void syncFsFile(const reg::FsPathResolution& sample);
    void collectKeysForFsFile(const reg::FsPathResolution& sample,
                              std::map<std::string, std::string>& out) const;

    std::string fileAbsPath(const reg::FsPathResolution& r) const;

    std::string m_rootDir;
};

} // namespace os

#endif

#ifndef OMNISHELL_CORE_JSON_REGISTRY_HPP
#define OMNISHELL_CORE_JSON_REGISTRY_HPP

#include "AbstractRegistry.hpp"

#include <string>

namespace os {

/**
 * JSON tree registry: both '/' and '.' map into nested JSON. The parent path + .json is the file;
 * the last slash-separated field (dot-separated) is the object path inside that file.
 * Example: foo/bar/cat.dog.k -> file <root>/foo/bar.json, { "cat": { "dog": { "k": "..." } } }.
 *
 * Root may be a directory (tree of .json files) or a single .json file (its stem is the top name).
 */
class JsonRegistry : public AbstractRegistry {
  public:
    explicit JsonRegistry(std::string root_path);

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

    void syncJsonFile(const std::string& jsonFileRel);
    void collectKeysForJsonFile(const std::string& jsonFileRel,
                                std::map<std::vector<std::string>, std::string>& out) const;

    std::string fileAbsPath(const std::string& jsonFileRel) const;

    std::string m_rootDir;
    /** If non-empty, only this file (relative name) under m_rootDir is used. */
    std::string m_singleJsonFile;
};

} // namespace os

#endif

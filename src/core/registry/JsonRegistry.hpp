#ifndef OMNISHELL_CORE_JSON_REGISTRY_HPP
#define OMNISHELL_CORE_JSON_REGISTRY_HPP

#include "IRegistry.hpp"

#include <map>
#include <string>

namespace os {

/**
 * Single-file flat JSON map { "a.b": "value", ... } (legacy format).
 */
class JsonRegistry : public IRegistry {
  public:
    explicit JsonRegistry(std::string json_file_path);

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

    bool parseFlatJson(const std::string& json);
    bool writeFlatJson() const;

    std::string m_path;
    mutable bool m_loaded{false};
    mutable std::map<std::string, std::string> m_data;
};

} // namespace os

#endif

#ifndef OMNISHELL_UI_ICON_THEME_HPP
#define OMNISHELL_UI_ICON_THEME_HPP

#include <bas/fmt/JsonForm.hpp>
#include <bas/ui/arch/ImageSet.hpp>

#include <string>

namespace os {

using IconMap = std::map<std::string, ImageSet>;

class IconTheme : public IJsonForm {

  public:
    const std::optional<ImageSet> getIcon(const std::string& app, const std::string& id) const;

    /** Always returns an ImageSet (empty if not found). */
    ImageSet icon(const std::string& app, const std::string& id) const;

    // Parse theme using boost::property_tree instead of boost::json parsing.
    bool loadFromJsonText(const std::string& jsonText);

  public:
    void jsonIn(boost::json::object& in,
                const JsonFormOptions& opts = JsonFormOptions::DEFAULT) override;
    void jsonOut(boost::json::object& obj,
                 const JsonFormOptions& opts = JsonFormOptions::DEFAULT) override;


  private:
    IconMap m_defaultIcons;
    std::map<std::string, IconMap> m_appIcons;
};

} // namespace os

#endif

#include "IconTheme.hpp"

#include <bas/ui/arch/ImageSet.hpp>
#include <bas/ui/arch/UIAction.hpp>

#include <wx/artprov.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <optional>
#include <sstream>

namespace os {

std::string kindToString(boost::json::kind k) {
    switch (k) {
    case boost::json::kind::object:
        return "object";
    case boost::json::kind::array:
        return "array";
    case boost::json::kind::string:
        return "string";
    case boost::json::kind::int64:
        return "int64";
    case boost::json::kind::uint64:
        return "uint64";
    case boost::json::kind::double_:
        return "double";
    case boost::json::kind::bool_:
        return "bool";
    case boost::json::kind::null:
        return "null";
    default:
        return "unknown";
    }
}

void IconTheme::jsonOut(boost::json::object& obj, const JsonFormOptions& opts) {
    (void)obj;
    (void)opts;
    // Not currently needed; theme files are read-only in-app.
}

ImageSet parseImageSet(const boost::json::value& in) {
    if (in.is_string()) {
        const auto& s = in.as_string();
        std::string_view path(s.data(), s.size());
        return ImageSet(Path(path));
    }
    if (in.is_object()) {
        auto obj = in.as_object();
        wxArtID artId;
        std::string assetPath;
        std::map<std::pair<int, int>, std::string> scales;

        for (auto& [key, value] : obj) {
            if (key == "wxartid") {
                artId = wxArtID(value.as_string().data());
            } else if (key == "asset") {
                assetPath = value.as_string().data();
            } else if (key.find("x") != std::string::npos) {
                size_t pos = key.find("x");
                if (pos != std::string::npos) {
                    std::string wstr(key.substr(0, pos).data(), pos);
                    std::string hstr(key.substr(pos + 1).data(), key.size() - (pos + 1));
                    size_t width = std::stoi(wstr);
                    size_t height = std::stoi(hstr);
                    scales[{width, height}] = value.as_string().data();
                }
            } else {
                logerror_fmt("IconTheme: invalid key %s, skipped", key.data());
            }
        }

        // assetPath.find_last_of(".");
        auto text = assetPath;
        if (text.empty()) {
            text = artId.ToStdString();
        }

        ImageSet icon(artId, std::optional<Path>(assetPath), text);
        for (auto& [scale, path] : scales) {
            icon.scale(scale.first, scale.second, Path(path.data()));
        }
        return icon;
    }

    logerror_fmt("IconTheme: invalid image set, skipped");
    return ImageSet();
}


IconMap parseIconMap(const boost::json::object& in) {
    IconMap map;
    for (auto& [_key, value] : in) {
        // std::string key = _key;
        std::string key(_key.data(), _key.size());
        map[key] = parseImageSet(value);
    }
    return map;
}

static ImageSet parseImageSetPtree(const boost::property_tree::ptree& node) {
    wxArtID artId;
    std::string assetPath;
    std::map<std::pair<int, int>, std::string> scales;

    for (const auto& child : node) {
        const std::string& key = child.first;
        const auto& v = child.second;

        if (key == "wxartid") {
            const std::string s = v.get_value<std::string>();
            artId = wxArtID(s.c_str());
        } else if (key == "asset") {
            assetPath = v.get_value<std::string>();
        } else if (key.find('x') != std::string::npos) {
            const size_t pos = key.find('x');
            if (pos != std::string::npos) {
                try {
                    const int w = std::stoi(key.substr(0, pos));
                    const int h = std::stoi(key.substr(pos + 1));
                    scales[{w, h}] = v.get_value<std::string>();
                } catch (...) {
                }
            }
        }
    }

    std::string text = assetPath;
    if (text.empty())
        text = artId.ToStdString();

    std::optional<Path> assetOpt;
    if (!assetPath.empty())
        assetOpt = Path(assetPath);

    ImageSet icon(artId, assetOpt, text);
    for (const auto& [scale, path] : scales) {
        icon.scale(scale.first, scale.second, Path(path));
    }
    return icon;
}

static IconMap parseIconMapPtree(const boost::property_tree::ptree& in) {
    IconMap map;
    for (const auto& kv : in) {
        const std::string& iconId = kv.first;
        const auto& value = kv.second;
        map[iconId] = parseImageSetPtree(value);
    }
    return map;
}
void IconTheme::jsonIn(boost::json::object& in, const JsonFormOptions& opts) {
    (void)opts;
    for (auto& [key, value] : in) {
        const std::string keyStr(key.data(), key.size());
        if (!value.is_object()) {
            logerror_fmt("IconTheme: expected object, got %s", kindToString(value.kind()).c_str());
            continue;
        }
         
        auto child= value.as_object();
        if (keyStr == "default") {
            m_defaultIcons = parseIconMap(child);
        } else {
            m_appIcons[keyStr] = parseIconMap(child);
        }
    }
}

const std::optional<ImageSet> IconTheme::getIcon(const std::string& app, const std::string& id) const {
    if (m_appIcons.find(app) != m_appIcons.end()) {
        auto& map = m_appIcons.at(app);
        auto it = map.find(id);
        if (it != map.end()) {
            return it->second;
        }
    }
    auto it = m_defaultIcons.find(id);
    if (it != m_defaultIcons.end()) {
        return it->second;
    }
    return std::nullopt;
}

ImageSet IconTheme::icon(const std::string& app, const std::string& id) const {
    const auto iconOpt = getIcon(app, id);
    return iconOpt ? *iconOpt : ImageSet();
}

bool IconTheme::loadFromJsonText(const std::string& jsonText) {
    using boost::property_tree::ptree;
    try {
        ptree root;
        std::stringstream ss(jsonText);
        boost::property_tree::read_json(ss, root);

        m_defaultIcons.clear();
        m_appIcons.clear();

        for (const auto& kv : root) {
            const std::string& key = kv.first;
            const ptree& child = kv.second;
            if (key == "default") {
                m_defaultIcons = parseIconMapPtree(child);
            } else {
                m_appIcons[key] = parseIconMapPtree(child);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace os
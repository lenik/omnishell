#include "FileAssociations.hpp"

#include "RegistryService.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <sstream>

namespace os {

std::optional<std::string> getFirstOpenWithModuleForExtension(std::string_view ext) {
    std::string e(ext);
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
    if (e.empty())
        return std::nullopt;

    const std::string key = "OmniShell.FileTypes.extension." + e;
    const std::string json = registry().getString(key, "");
    if (json.empty())
        return std::nullopt;

    try {
        std::stringstream ss(json);
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(ss, pt);
        const auto ow = pt.get_child_optional("openwith");
        if (!ow)
            return std::nullopt;
        for (const auto& kv : *ow) {
            const std::string& uri = kv.second.data();
            if (!uri.empty())
                return uri;
        }
    } catch (...) {
    }
    return std::nullopt;
}

} // namespace os

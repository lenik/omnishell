#include "RegistryKeyUtil.hpp"

#include <set>

namespace os::reg {

std::vector<std::string> listChildKeys(const std::map<std::string, std::string>& data,
                                       const std::string& node_key, bool full_key) {
    const std::string pref = node_key.empty() ? "" : (node_key + ".");
    std::set<std::string> ordered;

    for (const auto& kv : data) {
        const std::string& key = kv.first;
        if (!pref.empty()) {
            if (key.rfind(pref, 0) != 0)
                continue;
            if (key.size() <= pref.size())
                continue;
        }
        std::string rest = pref.empty() ? key : key.substr(pref.size());
        size_t dot = rest.find('.');
        std::string seg = dot == std::string::npos ? rest : rest.substr(0, dot);
        if (seg.empty())
            continue;
        std::string out = full_key ? (node_key.empty() ? seg : node_key + "." + seg) : seg;
        ordered.insert(out);
    }

    return std::vector<std::string>(ordered.begin(), ordered.end());
}

std::vector<std::string> keysForDelTree(const std::map<std::string, std::string>& data,
                                        const std::string& node_key) {
    std::vector<std::string> out;
    const std::string dot = node_key + ".";
    for (const auto& kv : data) {
        const std::string& k = kv.first;
        if (k == node_key || k.rfind(dot, 0) == 0)
            out.push_back(k);
    }
    return out;
}

} // namespace os::reg

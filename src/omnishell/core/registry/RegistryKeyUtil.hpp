#ifndef OMNISHELL_CORE_REGISTRY_KEY_UTIL_HPP
#define OMNISHELL_CORE_REGISTRY_KEY_UTIL_HPP

#include <map>
#include <string>
#include <vector>

namespace os::reg {

std::vector<std::string> listChildKeys(const std::map<std::string, std::string>& data,
                                       const std::string& node_key, bool full_key);

/** Keys to remove: exact node_key and any key starting with node_key + "." */
std::vector<std::string> keysForDelTree(const std::map<std::string, std::string>& data,
                                        const std::string& node_key);

} // namespace os::reg

#endif

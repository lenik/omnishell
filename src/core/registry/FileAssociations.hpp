#ifndef OMNISHELL_CORE_FILE_ASSOCIATIONS_HPP
#define OMNISHELL_CORE_FILE_ASSOCIATIONS_HPP

#include <optional>
#include <string>
#include <string_view>

namespace os {

/**
 * Reads OmniShell.FileTypes.extension.<ext> and returns the first module URI in `openwith`
 * (JSON object iteration order).
 */
std::optional<std::string> getFirstOpenWithModuleForExtension(std::string_view extLowerOrAny);

} // namespace os

#endif

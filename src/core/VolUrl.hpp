#ifndef OMNISHELL_CORE_VOL_URL_HPP
#define OMNISHELL_CORE_VOL_URL_HPP

#include <bas/volume/VolumeFile.hpp>

#include <string_view>

class VolumeManager;

namespace os {

/** Parse vol://<volumeIndex>/<path> into a VolumeFile. Returns false on invalid URL or volume index. */
bool parseVolUrl(VolumeManager* vm, std::string_view url, VolumeFile& out);

} // namespace os

#endif

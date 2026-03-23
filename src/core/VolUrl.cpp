#include "VolUrl.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <cctype>
#include <string>

namespace os {

bool parseVolUrl(VolumeManager* vm, std::string_view url, VolumeFile& out) {
    constexpr std::string_view kPfx = "vol://";
    if (!vm || url.size() < kPfx.size() + 2 || url.compare(0, kPfx.size(), kPfx) != 0)
        return false;

    size_t i = kPfx.size();
    const size_t n = url.size();
    if (i >= n || !std::isdigit(static_cast<unsigned char>(url[i])))
        return false;
    size_t startDigit = i;
    while (i < n && std::isdigit(static_cast<unsigned char>(url[i])))
        ++i;
    if (i == startDigit)
        return false;

    int idx = 0;
    for (size_t d = startDigit; d < i; ++d) {
        idx = idx * 10 + static_cast<int>(url[d] - '0');
        if (idx < 0)
            return false;
    }
    if (idx < 0 || static_cast<size_t>(idx) >= vm->getVolumeCount())
        return false;

    Volume* v = vm->getVolume(static_cast<size_t>(idx));
    if (!v)
        return false;

    std::string path = i < n ? std::string(url.substr(i)) : std::string("/");
    if (path.empty())
        path = "/";
    if (path[0] != '/')
        path.insert(path.begin(), '/');
    try {
        path = v->normalize(path, false);
    } catch (...) {
        return false;
    }

    out = VolumeFile(v, std::move(path));
    return true;
}

} // namespace os

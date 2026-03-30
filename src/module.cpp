#include <bas/log/deflog.h>
#include <bas/proc/DefAssets.hpp>
#include <bas/proc/AssetsRegistry.hpp>

extern "C" {

define_logger();

define_zip_assets(_omni, omni_assets);

}

namespace {

struct OmniAssetsRegistrar {
    OmniAssetsRegistrar() {
        AssetsRegistry::pushLayer(_omni_assets.get());
    }
} omni_assets_registrar;

}
#include <bas/proc/AssetsRegistry.hpp>
#include <bas/proc/DefAssets.hpp>

#include <bas/log/deflog.h>

extern "C" {

define_logger();

define_zip_assets(omni, omni_assets);

}

[[gnu::used]] __attribute__((used))
struct OmniAssetsRegistrar {
    OmniAssetsRegistrar() { AssetsRegistry::pushLayer(omni_assets.get()); }
} omni_assets_registrar;

extern "C"
__attribute__((constructor))
void omnishell_init() {
}

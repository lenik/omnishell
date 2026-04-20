#include <bas/log/deflog.h>
#include <bas/proc/AssetsRegistry.hpp>
#include <bas/proc/DefAssets.hpp>

extern "C" {

define_logger();

define_zip_assets(omni, omni_assets);

}

namespace {

bool omni_assets_registered = false;

void ensureRegisteredImpl() {
    if (omni_assets_registered)
        return;
    omni_assets_registered = true;
    AssetsRegistry::pushLayer(omni_assets.get());
}

struct OmniAssetsRegistrar {
    OmniAssetsRegistrar() { ensureRegisteredImpl(); }
};

[[gnu::used]] __attribute__((used)) OmniAssetsRegistrar omni_assets_registrar;

} // namespace

extern "C" void omnishell_ensure_omni_assets_registered() {
    ensureRegisteredImpl();
}
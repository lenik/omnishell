#include <bas/log/deflog.h>
#include <bas/proc/DefAssets.hpp>
#include <bas/volume/OverlapVolume.hpp>

#define ASSETS_NAME bas_cpp
#include <bas/proc/UseAssets.hpp>

#undef ASSETS_NAME
#define ASSETS_NAME bas_ui
#include <bas/proc/UseAssets.hpp>

#include <memory>

define_logger();

define_zip_assets(_omni, omni_assets);

extern "C" {

std::unique_ptr<OverlapVolume> omni_assets = std::make_unique<OverlapVolume>(
    std::vector<Volume*>{bas_cpp_assets.get(), bas_ui_assets.get(), _omni_assets.get()});
}
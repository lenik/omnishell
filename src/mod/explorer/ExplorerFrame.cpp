#include "ExplorerFrame.hpp"

#include <bas/volume/VolumeManager.hpp>

namespace os {

ExplorerFrame::ExplorerFrame(App* app, VolumeManager* vm, Volume* volume, std::string dir)
    : uiFrame("Explorer")
    , m_body(vm, volume, std::move(dir)) {
    (void)app;
    addFragment(&m_body);
    createView();
}

void ExplorerFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

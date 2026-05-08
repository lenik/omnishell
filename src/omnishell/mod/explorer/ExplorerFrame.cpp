#include "ExplorerFrame.hpp"

#include <bas/volume/VolumeManager.hpp>

namespace os {

ExplorerFrame::ExplorerFrame(App* app, VolumeManager* vm, Volume* volume, std::string dir)
    : uiFrame("Explorer")
    , m_body(vm) {
    (void)app;
    m_body.setOpenTarget(volume, dir.empty() ? "/" : std::move(dir));
    addFragment(&m_body);
    createView();
}

wxWindow* ExplorerFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

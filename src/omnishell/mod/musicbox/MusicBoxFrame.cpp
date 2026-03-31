#include "MusicBoxFrame.hpp"

#include "../../core/App.hpp"

namespace os {

MusicBoxFrame::MusicBoxFrame(App* app, VolumeManager* vm, std::string title)
    : uiFrame(std::move(title))
    , m_body(vm) {
    (void)app;
    addFragment(&m_body);
    createView();
}

void MusicBoxFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os


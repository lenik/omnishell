#include "MediaPlayerFrame.hpp"

namespace os {

MediaPlayerFrame::MediaPlayerFrame(VolumeManager* vm, std::string title)
    : uiFrame(std::move(title))
    , m_body(vm) {
    addFragment(&m_body);
    createView();
}

wxWindow* MediaPlayerFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

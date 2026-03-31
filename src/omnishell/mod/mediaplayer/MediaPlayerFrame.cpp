#include "MediaPlayerFrame.hpp"

namespace os {

MediaPlayerFrame::MediaPlayerFrame(VolumeManager* vm, std::string title)
    : uiFrame(std::move(title))
    , m_body(vm) {
    addFragment(&m_body);
    createView();
}

void MediaPlayerFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

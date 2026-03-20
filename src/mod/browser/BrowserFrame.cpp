#include "BrowserFrame.hpp"

namespace os {

BrowserFrame::BrowserFrame(VolumeManager* vm, std::string title)
    : uiFrame(title)
    , m_body(vm) {
    addFragment(&m_body);
    createView();
}

void BrowserFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

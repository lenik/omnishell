#include "StopWatchFrame.hpp"

namespace os {

StopWatchFrame::StopWatchFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body() {
    (void)app;
    addFragment(&m_body);
    createView();
}

void StopWatchFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

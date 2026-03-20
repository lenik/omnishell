#include "BackgroundSettingsFrame.hpp"

namespace os {

BackgroundSettingsFrame::BackgroundSettingsFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body() {
    (void)app;
    addFragment(&m_body);
    createView();
}

void BackgroundSettingsFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

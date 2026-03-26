#include "SoundRecorderFrame.hpp"

namespace os {

SoundRecorderFrame::SoundRecorderFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body() {
    (void)app;
    addFragment(&m_body);
    createView();
}

void SoundRecorderFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

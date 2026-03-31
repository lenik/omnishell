#include "CameraFrame.hpp"

namespace os {

CameraFrame::CameraFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body() {
    (void)app;
    addFragment(&m_body);
    createView();
}

void CameraFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os


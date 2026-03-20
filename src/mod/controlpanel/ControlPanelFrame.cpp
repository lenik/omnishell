#include "ControlPanelFrame.hpp"

namespace os {

ControlPanelFrame::ControlPanelFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body(app) {
    addFragment(&m_body);
    createView();
}

void ControlPanelFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

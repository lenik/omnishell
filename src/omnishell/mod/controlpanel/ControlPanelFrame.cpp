#include "ControlPanelFrame.hpp"

namespace os {

ControlPanelFrame::ControlPanelFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body(app) {
    addFragment(&m_body);
    createView();
}

wxWindow* ControlPanelFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

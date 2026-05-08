#include "PaintFrame.hpp"

namespace os {

PaintFrame::PaintFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body() {
    (void)app;
    addFragment(&m_body);
    createView();
}

wxWindow* PaintFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

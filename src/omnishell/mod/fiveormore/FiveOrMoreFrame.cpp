#include "FiveOrMoreFrame.hpp"

namespace os {

FiveOrMoreFrame::FiveOrMoreFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body(app) {
    addFragment(&m_body);
    createView();
}

wxWindow* FiveOrMoreFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

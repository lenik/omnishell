#include "FiveOrMoreFrame.hpp"

namespace os {

FiveOrMoreFrame::FiveOrMoreFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body(app) {
    addFragment(&m_body);
    createView();
}

void FiveOrMoreFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

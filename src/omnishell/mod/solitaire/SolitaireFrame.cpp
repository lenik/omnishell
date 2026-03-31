#include "SolitaireFrame.hpp"

namespace os {

SolitaireFrame::SolitaireFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body(app) {
    addFragment(&m_body);
    createView();
}

void SolitaireFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

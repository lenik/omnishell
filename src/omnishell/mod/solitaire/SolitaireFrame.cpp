#include "SolitaireFrame.hpp"

namespace os {

SolitaireFrame::SolitaireFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body(app) {
    addFragment(&m_body);
    createView();
}

wxWindow* SolitaireFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

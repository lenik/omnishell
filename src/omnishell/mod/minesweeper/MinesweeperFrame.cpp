#include "MinesweeperFrame.hpp"

namespace os {

MinesweeperFrame::MinesweeperFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body(app) {
    addFragment(&m_body);
    createView();
}

wxWindow* MinesweeperFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

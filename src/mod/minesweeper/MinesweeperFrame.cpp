#include "MinesweeperFrame.hpp"

namespace os {

MinesweeperFrame::MinesweeperFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body(app) {
    addFragment(&m_body);
    createView();
}

void MinesweeperFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

#include "SudokuFrame.hpp"

namespace os {

SudokuFrame::SudokuFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body(app) {
    addFragment(&m_body);
    createView();
}

void SudokuFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

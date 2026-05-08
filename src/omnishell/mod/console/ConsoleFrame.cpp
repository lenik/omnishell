#include "ConsoleFrame.hpp"

#include "../../core/App.hpp"

namespace os {

ConsoleFrame::ConsoleFrame(App* app)
    : uiFrame("Console")
    , m_body(app) {
    (void)app;
    SetName(wxT("console"));
    addFragment(&m_body);
    createView();
    SetSize(wxSize(900, 520));
}

wxWindow* ConsoleFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

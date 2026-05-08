#include "BackgroundSettingsFrame.hpp"
#include <optional>

namespace os {

BackgroundSettingsFrame::BackgroundSettingsFrame(App* app, std::string title)
    : uiFrame(title, std::nullopt, //
              nullptr, wxID_ANY,   //
              wxDefaultPosition, //
              wxSize(480, 640)),
      m_body() {
    (void)app;
    addFragment(&m_body);
    createView();
}

wxWindow* BackgroundSettingsFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

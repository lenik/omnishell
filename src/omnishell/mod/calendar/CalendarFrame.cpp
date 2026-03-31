#include "CalendarFrame.hpp"

namespace os {

CalendarFrame::CalendarFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body() {
    (void)app;
    addFragment(&m_body);
    createView();
}

void CalendarFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

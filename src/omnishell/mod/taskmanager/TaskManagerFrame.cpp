#include "TaskManagerFrame.hpp"

namespace os {

TaskManagerFrame::TaskManagerFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body() {
    (void)app;
    addFragment(&m_body);
    createView();
}

wxWindow* TaskManagerFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

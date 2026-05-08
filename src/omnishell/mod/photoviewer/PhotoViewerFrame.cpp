#include "PhotoViewerFrame.hpp"

namespace os {

PhotoViewerFrame::PhotoViewerFrame(App* app, std::string title)
    : uiFrame(std::move(title))
    , m_body() {
    (void)app;
    addFragment(&m_body);
    createView();
}

wxWindow* PhotoViewerFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os


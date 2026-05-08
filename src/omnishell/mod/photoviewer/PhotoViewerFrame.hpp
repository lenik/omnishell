#ifndef OMNISHELL_MOD_PHOTO_VIEWER_FRAME_HPP
#define OMNISHELL_MOD_PHOTO_VIEWER_FRAME_HPP

#include "PhotoViewerBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class PhotoViewerFrame : public uiFrame {
public:
    PhotoViewerFrame(App* app, std::string title);
    ~PhotoViewerFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

    PhotoViewerBody& body() { return m_body; }

private:
    PhotoViewerBody m_body;
};

} // namespace os

#endif // OMNISHELL_MOD_PHOTO_VIEWER_FRAME_HPP


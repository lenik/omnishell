#ifndef OMNISHELL_MOD_CAMERA_CAMERAFRAME_HPP
#define OMNISHELL_MOD_CAMERA_CAMERAFRAME_HPP

#include "CameraBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

namespace os {

class CameraFrame : public uiFrame {
public:
    CameraFrame(App* app, std::string title);
    ~CameraFrame() override = default;

    void createFragmentView(CreateViewContext* ctx) override;

    CameraBody& body() { return m_body; }

private:
    CameraBody m_body;
};

} // namespace os

#endif // OMNISHELL_MOD_CAMERA_CAMERAFRAME_HPP


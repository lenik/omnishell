#ifndef OMNISHELL_MOD_CAMERA_CAMERAAPP_HPP
#define OMNISHELL_MOD_CAMERA_CAMERAAPP_HPP

#include "../../core/Module.hpp"

namespace os {

class App;

class CameraApp : public Module {
public:
    explicit CameraApp(CreateModuleContext* ctx);
    ~CameraApp() override;

    void install() override;

    void initializeMetadata();
    ProcessPtr run(const RunConfig& config) override;

private:
    App* m_app{nullptr};
};

} // namespace os

#endif // OMNISHELL_MOD_CAMERA_CAMERAAPP_HPP


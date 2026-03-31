#include "CameraApp.hpp"

#include "CameraFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../ui/ThemeStyles.hpp"

using namespace ThemeStyles;

namespace os {

namespace {
constexpr const char* kCameraModuleUri = "omnishell.Camera";
}

CameraApp::CameraApp(CreateModuleContext* ctx)
    : Module(ctx)
    , m_app(ctx->getApp()) {
    initializeMetadata();
}

CameraApp::~CameraApp() = default;

void CameraApp::install() {}

void CameraApp::initializeMetadata() {
    uri = kCameraModuleUri;
    name = "camera";
    label = "Camera";
    description = "Photo and video capture utility";
    doc = "Cheese-like photo/video UI with a recent gallery. Capture is implemented as copy-from-file (no webcam dependency).";
    categoryId = ID_CATEGORY_ACCESSORIES;
    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("camera", "icon");
}

ProcessPtr CameraApp::run(const RunConfig& config) {
    (void)config;

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new CameraFrame(m_app, "Camera");
    frame->SetSize(wxSize(860, 620));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

OMNISHELL_REGISTER_MODULE(kCameraModuleUri, CameraApp)

} // namespace os


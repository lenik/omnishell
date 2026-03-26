#include "PaintApp.hpp"

#include "PaintFrame.hpp"

#include "../../ui/ThemeStyles.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../core/VolUrl.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/mstream.h>

namespace {
constexpr const char* kPaintModuleUri = "omnishell.Paint";
}

namespace os {
using namespace ThemeStyles;

OMNISHELL_REGISTER_MODULE(kPaintModuleUri, PaintApp)

PaintApp::PaintApp(CreateModuleContext* ctx) : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

PaintApp::~PaintApp() {}

void PaintApp::initializeMetadata() {
    uri = kPaintModuleUri;
    name = "paint";
    label = "Paint";
    description = "Simple drawing canvas";
    doc = "Basic paint application.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("paint", "icon");
}

ProcessPtr PaintApp::run(const RunConfig& config) {
    if (m_app && m_app->volumeManager && !config.args.empty()) {
        Volume* anchor = m_app->volumeManager->getDefaultVolume();
        if (anchor) {
            VolumeFile vf(anchor, "/");
            if (parseVolUrl(m_app->volumeManager.get(), config.args[0], vf))
                return openImage(m_app->volumeManager.get(), vf);
        }
    }

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new PaintFrame(m_app, "Paint");
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

ProcessPtr PaintApp::openImage(VolumeManager* volumeManager, VolumeFile file) {
    (void)volumeManager;

    auto proc = std::make_shared<Process>();
    proc->uri = kPaintModuleUri;
    proc->name = "paint";
    proc->label = "Paint";
    proc->icon = os::app.getIconTheme()->icon("paint", "icon");

    auto* frame = new PaintFrame(&app, "Paint");

    try {
        auto data = file.readFile();
        if (!data.empty()) {
            wxMemoryInputStream ms(data.data(), data.size());
            wxImage img(ms, wxBITMAP_TYPE_ANY);
            if (img.IsOk()) {
                frame->body().loadImage(img);
            }
        }
    } catch (...) {
    }

    frame->Centre();
    frame->Show(true);

    proc->addWindow(frame);
    return proc;
}

} // namespace os

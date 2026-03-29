#include "NotepadApp.hpp"

#include "NotepadFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../core/VolUrl.hpp"
#include "../../ui/ThemeStyles.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

using namespace ThemeStyles;
namespace os {

namespace {
constexpr const char* kNotepadModuleUri = "omnishell.Notepad";
}

OMNISHELL_REGISTER_MODULE(kNotepadModuleUri, NotepadApp)

NotepadApp::NotepadApp(CreateModuleContext* ctx) : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

NotepadApp::~NotepadApp() {}

void NotepadApp::initializeMetadata() {
    uri = kNotepadModuleUri;
    name = "notepad";
    label = "Notepad";
    description = "Simple text editor";
    doc = "A basic text editor for viewing and editing text files.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("notepad", "icon");
}

ProcessPtr NotepadApp::run(const RunConfig& config) {
    if (m_app && m_app->volumeManager && !config.args.empty()) {
        Volume* anchor = m_app->volumeManager->getDefaultVolume();
        if (anchor) {
            VolumeFile vf(anchor, "/");
            if (parseVolUrl(m_app->volumeManager.get(), config.args[0], vf))
                return open(m_app->volumeManager.get(), vf);
        }
    }

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new NotepadFrame(m_app, "Notepad");
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

ProcessPtr NotepadApp::open(VolumeManager* volumeManager, VolumeFile file) {
    (void)volumeManager;

    auto proc = std::make_shared<Process>();
    proc->uri = kNotepadModuleUri;
    proc->name = "notepad";
    proc->label = "Notepad";
    proc->icon = os::app.getIconTheme()->icon("notepad", "icon");

    auto* frame = new NotepadFrame(&app, "Notepad");
    frame->body().openFile(file);
    frame->Centre();
    frame->Show(true);

    proc->addWindow(frame);
    return proc;
}

} // namespace os

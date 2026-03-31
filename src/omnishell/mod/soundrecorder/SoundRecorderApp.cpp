#include "SoundRecorderApp.hpp"

#include "SoundRecorderFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../ui/ThemeStyles.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <string>

using namespace ThemeStyles;
namespace os {

namespace {
constexpr const char* kSoundRecorderModuleUri = "omnishell.SoundRecorder";
}

OMNISHELL_REGISTER_MODULE(kSoundRecorderModuleUri, SoundRecorderApp)

SoundRecorderApp::SoundRecorderApp(CreateModuleContext* ctx) : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

SoundRecorderApp::~SoundRecorderApp() {}

void SoundRecorderApp::initializeMetadata() {
    uri = kSoundRecorderModuleUri;
    name = "soundrecorder";
    label = "Sound Recorder";
    description = "Audio recording utility";
    doc = "Record audio from your microphone and save as audio files.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("soundrecorder", "icon");
}

ProcessPtr SoundRecorderApp::run(const RunConfig& config) {
    (void)config;

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new SoundRecorderFrame(m_app, std::string("Sound Recorder"));
    frame->SetSize(wxSize(400, 300));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

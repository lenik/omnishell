#include "MusicBoxApp.hpp"

#include "MusicBoxFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../core/registry/RegistryService.hpp"
#include "../../shell/Shell.hpp"

#include "../../ui/ThemeStyles.hpp"

using namespace ThemeStyles;

namespace os {

constexpr const char* kMusicBoxUri = "omnishell.MusicBox";

MusicBoxApp::MusicBoxApp(CreateModuleContext* ctx)
    : Module(ctx)
    , m_app(ctx->getApp()) {
    initializeMetadata();
}

MusicBoxApp::~MusicBoxApp() = default;

void MusicBoxApp::install() {
    Module::install();

    // Provide open-with mapping for audio extensions.
    IRegistry& r = registry();
    auto ensure = [&r](const std::string& key, const char* value) {
        if (!r.has(key))
            r.set(key, std::string(value));
    };

    constexpr const char* kMusicBoxFileTypeJson =
        R"({"openwith":{"MusicBox":"omnishell.MusicBox"},"icon":{"asset":"streamline-vectors/core/pop/entertainment/music-note-1.svg"}})";

    static const char* kAudioExts[] = {"mp3",  "wav",  "flac", "ogg",  "oga",  "opus", "m4a",  "aac",
                                       "wma",  "mid",  "midi", "kar",  "aiff", "aif",  "mpc",  "ape",
                                       "wv",   "dsf",  "dff"};
    for (const char* ext : kAudioExts) {
        ensure(std::string("OmniShell.FileTypes.extension.") + ext, kMusicBoxFileTypeJson);
    }

    r.save();
}

void MusicBoxApp::initializeMetadata() {
    uri = kMusicBoxUri;
    name = "musicbox";
    label = "MusicBox";
    description = "Foobar-like music player";
    doc = "Library + playlist + transport controls over VFS media streaming.";
    categoryId = ID_CATEGORY_ACCESSORIES;
    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("musicbox", "icon");
}

ProcessPtr MusicBoxApp::run(const RunConfig& config) {
    (void)config;

    VolumeManager* vm = ShellApp::getInstance() ? ShellApp::getInstance()->getVolumeManager() : nullptr;
    if (!vm) {
        wxMessageBox("No volume manager", "MusicBox", wxOK | wxICON_ERROR);
        return nullptr;
    }

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new MusicBoxFrame(m_app, vm, "MusicBox");
    frame->SetSize(wxSize(980, 640));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os
namespace os {
OMNISHELL_REGISTER_MODULE(kMusicBoxUri, MusicBoxApp)
} // namespace os


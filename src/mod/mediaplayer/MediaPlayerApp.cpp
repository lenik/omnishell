#include "MediaPlayerApp.hpp"

#include "MediaPlayerFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../core/registry/RegistryService.hpp"
#include "../../core/VolUrl.hpp"
#include "../../shell/Shell.hpp"

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/msgdlg.h>

#include <string>

namespace os {

namespace {
constexpr const char* kMediaPlayerUri = "omnishell.MediaPlayer";

/** First handler in openwith is used by Explorer (see FileAssociations). */
constexpr const char* kMediaFileTypeJson =
    R"({"openwith":{"MediaPlayer":"omnishell.MediaPlayer"},"icon":{"asset":"streamline-vectors/core/pop/entertainment/camera-video.svg"}})";

constexpr const char* kMediaMimeOpenWith = R"({"openwith":{"MediaPlayer":"omnishell.MediaPlayer"}})";

static const char* kMediaExtensions[] = {
    // Audio
    "mp3",  "wav",  "flac", "ogg",  "oga",  "opus", "m4a",  "aac",  "wma",  "mid",  "midi", "kar",
    "aiff", "aif",  "mpc",  "ape",  "wv",   "dsf",  "dff",
    // Video
    "mp4",  "m4v",  "mkv",  "webm", "avi",  "mov",  "wmv",  "mpg",  "mpeg", "ogv",  "3gp",  "ts",
    "mts",  "m2ts", "asf",  "divx", "vob",
};

static int volumeIndexFor(VolumeManager* vm, Volume* vol) {
    if (!vm || !vol)
        return -1;
    for (size_t i = 0; i < vm->getVolumeCount(); ++i) {
        if (vm->getVolume(i) == vol)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace

OMNISHELL_REGISTER_MODULE(kMediaPlayerUri, MediaPlayerApp)

MediaPlayerApp::MediaPlayerApp(CreateModuleContext* ctx)
    : Module(ctx)
    , m_app(ctx->getApp()) {
    (void)m_app;
    initializeMetadata();
}

MediaPlayerApp::~MediaPlayerApp() = default;

void MediaPlayerApp::initializeMetadata() {
    uri = kMediaPlayerUri;
    name = "mediaplayer";
    label = "Media Player";
    description = "Play audio/video from volumes via the VFS HTTP bridge";
    doc = "Uses the shell VFS daemon (HTTP) to stream media from volume paths.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("mediaplayer", "icon");
}

void MediaPlayerApp::install() {
    Module::install();
    IRegistry& r = registry();

    auto ensure = [&r](const std::string& key, const char* value) {
        if (!r.has(key))
            r.set(key, std::string(value));
    };

    for (const char* ext : kMediaExtensions) {
        ensure(std::string("OmniShell.FileTypes.extension.") + ext, kMediaFileTypeJson);
    }

    ensure("OmniShell.FileTypes.mime-type.audio_mpeg", kMediaMimeOpenWith);
    ensure("OmniShell.FileTypes.mime-type.audio_mp4", kMediaMimeOpenWith);
    ensure("OmniShell.FileTypes.mime-type.audio_x-m4a", kMediaMimeOpenWith);
    ensure("OmniShell.FileTypes.mime-type.audio_wav", kMediaMimeOpenWith);
    ensure("OmniShell.FileTypes.mime-type.audio_flac", kMediaMimeOpenWith);
    ensure("OmniShell.FileTypes.mime-type.audio_ogg", kMediaMimeOpenWith);
    ensure("OmniShell.FileTypes.mime-type.video_mp4", kMediaMimeOpenWith);
    ensure("OmniShell.FileTypes.mime-type.video_webm", kMediaMimeOpenWith);
    ensure("OmniShell.FileTypes.mime-type.video_x-msvideo", kMediaMimeOpenWith);
    ensure("OmniShell.FileTypes.mime-type.video_quicktime", kMediaMimeOpenWith);

    r.save();
}

ProcessPtr MediaPlayerApp::run(const RunConfig& config) {
    VolumeManager* vm = ShellApp::getInstance() ? ShellApp::getInstance()->getVolumeManager() : nullptr;
    if (!vm) {
        wxMessageBox("No volume manager", "Media Player", wxOK | wxICON_ERROR);
        return nullptr;
    }

    if (!config.args.empty()) {
        Volume* anchor = vm->getDefaultVolume();
        if (anchor) {
            VolumeFile vf(anchor, "/");
            if (parseVolUrl(vm, config.args[0], vf))
                return open(vm, vf);
        }
    }

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new MediaPlayerFrame(vm, "Media Player");
    frame->SetSize(wxSize(560, 420));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

ProcessPtr MediaPlayerApp::open(VolumeManager* vm, VolumeFile file) {
    if (!vm) {
        wxMessageBox("No volume manager", "Media Player", wxOK | wxICON_ERROR);
        return nullptr;
    }
    Volume* v = file.getVolume();
    const int idx = volumeIndexFor(vm, v);
    if (idx < 0) {
        wxMessageBox("Could not resolve volume for this file.", "Media Player", wxOK | wxICON_WARNING);
        return nullptr;
    }
    std::string path = file.getPath();
    if (path.empty())
        path = "/";

    auto proc = std::make_shared<Process>();
    proc->uri = kMediaPlayerUri;
    proc->name = "mediaplayer";
    proc->label = "Media Player";
    proc->icon = os::app.getIconTheme()->icon("mediaplayer", "icon");

    auto* frame = new MediaPlayerFrame(vm, "Media Player");
    frame->body().openVolumePath(static_cast<size_t>(idx), path);
    frame->SetSize(wxSize(560, 420));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

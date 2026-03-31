#include "MediaPlayerBody.hpp"

#include "../../core/App.hpp"
#include "../../shell/Shell.hpp"
#include "../../ui/ThemeStyles.hpp"
#include "../../ui/dialogs/ChooseFileDialog.hpp"
#include "../../wx/artprovs.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/bitmap.h>
#include <wx/bmpbuttn.h>
#include <wx/dcclient.h>
#include <wx/log.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/uri.h>

#if HAVE_WX_MEDIA
#include <wx/mediactrl.h>
#endif

#include <algorithm>
#include <cmath>
#include <string_view>

using namespace ThemeStyles;

namespace os {

namespace {

enum {
    ID_GROUP_MEDIA = uiFrame::ID_APP_HIGHEST + 1,
    ID_OPEN,
    ID_PLAY,
    ID_PAUSE,
    ID_STOP,
    ID_STATE_REPEAT,
    ID_STATE_AUDIO_SPECTRUM,
};

constexpr int kControlIconPx = 22;
constexpr int kVizTimerMs = 45;
constexpr int kBarCount = 40;

static int volumeIndexFor(VolumeManager* vm, Volume* vol) {
    if (!vm || !vol)
        return -1;
    for (size_t i = 0; i < vm->getVolumeCount(); ++i) {
        if (vm->getVolume(i) == vol)
            return static_cast<int>(i);
    }
    return -1;
}

static wxString utf8Wx(const std::string& s) { return wxString::FromUTF8(s.data(), s.size()); }

static wxString displayNameFromPath(const std::string& path) {
    if (path.empty())
        return "Media Player";
    size_t last = path.rfind('/');
    if (last == std::string::npos)
        return utf8Wx(path);
    if (last + 1 >= path.size())
        return "Media Player";
    return utf8Wx(path.substr(last + 1));
}

static std::string stripVolumeSlashes(std::string rel) {
    while (!rel.empty() && rel.front() == '/')
        rel.erase(rel.begin());
    return rel;
}

static bool extensionIsAudio(std::string_view path) {
    size_t dot = path.rfind('.');
    if (dot == std::string_view::npos || dot + 1 >= path.size())
        return false;
    std::string ext(path.substr(dot + 1));
    for (auto& c : ext) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }
    static const char* kAudio[] = {"mp3", "wav", "flac", "ogg",  "oga", "opus", "m4a",
                                   "aac", "wma", "mid",  "midi", "kar", "aiff", "aif",
                                   "mpc", "ape", "wv",   "dsf",  "dff"};
    for (const char* a : kAudio) {
        if (ext == a)
            return true;
    }
    return false;
}

} // namespace

/** Synthetic waveform / spectrum (no raw PCM from wxMediaCtrl). */
class MediaAudioVisualizer : public wxPanel {
  public:
    explicit MediaAudioVisualizer(wxWindow* parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE) {
        SetDoubleBuffered(true);
        SetMinSize(wxSize(-1, 96));
        Bind(wxEVT_PAINT, &MediaAudioVisualizer::onPaint, this);
    }

    void setSpectrumMode(bool on) {
        if (m_spectrumMode == on)
            return;
        m_spectrumMode = on;
        Refresh(false);
    }

    void syncPlayback(bool playing, wxLongLong tellMs) {
        m_playing = playing;
        m_tellMs = tellMs;
    }

  private:
    void onPaint(wxPaintEvent&) {
        wxPaintDC dc(this);
        wxSize sz = GetClientSize();
        if (sz.x < 2 || sz.y < 2)
            return;

        wxColour bg = GetBackgroundColour();
        if (!bg.IsOk())
            bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        dc.SetBrush(wxBrush(bg));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, sz.x, sz.y);

        const double t = static_cast<double>(m_tellMs.GetValue()) * 0.001;
        const int midY = sz.y / 2;
        const wxColour fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        dc.SetPen(wxPen(m_playing ? fg : fg.ChangeLightness(150), 1));

        if (m_spectrumMode) {
            const double dt = t * 0.9;
            for (int i = 0; i < kBarCount; ++i) {
                double phase = dt * (1.2 + i * 0.11) + i * 0.7;
                double target = 0.12 + 0.88 * (0.5 + 0.5 * std::sin(phase)) *
                                           (0.65 + 0.35 * std::sin(dt * 0.4 + i * 0.2));
                if (!m_playing)
                    target *= 0.15;
                m_barSmooth[i] += (target - m_barSmooth[i]) * (m_playing ? 0.28 : 0.5);
                int h = static_cast<int>(m_barSmooth[i] * (sz.y - 8));
                h = std::clamp(h, 2, sz.y - 4);
                int x0 = i * sz.x / kBarCount + 1;
                int w = std::max(1, sz.x / kBarCount - 2);
                dc.SetBrush(wxBrush(fg.ChangeLightness(m_playing ? 95 : 130)));
                dc.SetPen(wxPen(fg.ChangeLightness(80), 1));
                dc.DrawRectangle(x0, sz.y - h - 2, w, h);
            }
        } else {
            const int step = std::max(2, sz.x / 280);
            auto sampleY = [&](int x) -> int {
                double u = x * 0.045 + t * 2.8;
                double v = std::sin(u) * 0.35 + std::sin(u * 1.7 + t) * 0.22 +
                           std::sin(u * 0.51 - t * 1.2) * 0.18;
                if (!m_playing)
                    v *= 0.2;
                int y = midY - static_cast<int>(v * (midY - 6));
                return std::clamp(y, 4, sz.y - 4);
            };
            wxPoint last(0, sampleY(0));
            for (int x = step; x < sz.x; x += step) {
                wxPoint cur(x, sampleY(x));
                dc.DrawLine(last.x, last.y, cur.x, cur.y);
                last = cur;
            }
        }
    }

    bool m_spectrumMode = false;
    bool m_playing = false;
    wxLongLong m_tellMs{0};
    double m_barSmooth[kBarCount]{};
};

static MediaAudioVisualizer* asViz(wxPanel* p) { return static_cast<MediaAudioVisualizer*>(p); }

MediaPlayerBody::MediaPlayerBody(VolumeManager* vm) : m_vm(vm) {
    const IconTheme* theme = app.getIconTheme();

    group(ID_GROUP_MEDIA, "media", "mediaplayer", 1000)
        .label("&Media")
        .description("Media player")
        .icon(theme->icon("mediaplayer", "group.media"))
        .install();

    int seq = 0;
    action(ID_OPEN, "media/mediaplayer", "open", seq++, "&Open...", "Open media file from volume")
        .icon(theme->icon("mediaplayer", "file.open"))
        .performFn([this](PerformContext* ctx) { onOpen(ctx); })
        .install();
    action(ID_PLAY, "media/mediaplayer", "play", seq++, "&Play", "Play")
        .icon(theme->icon("mediaplayer", "media.play"))
        .performFn([this](PerformContext* ctx) { onPlay(ctx); })
        .install();
    action(ID_PAUSE, "media/mediaplayer", "pause", seq++, "&Pause", "Pause")
        .icon(theme->icon("mediaplayer", "media.pause"))
        .performFn([this](PerformContext* ctx) { onPause(ctx); })
        .install();
    action(ID_STOP, "media/mediaplayer", "stop", seq++, "&Stop", "Stop")
        .icon(theme->icon("mediaplayer", "media.stop"))
        .performFn([this](PerformContext* ctx) { onStop(ctx); })
        .install();

    state(ID_STATE_REPEAT, "media/mediaplayer", "repeat", 10)
        .label("Repeat")
        .description("Repeat when playback reaches the end")
        .stateType(UIStateType::BOOL)
        .valueDescriptorFn([this](const UIStateVariant& nv) -> UIStateValueDescriptor {
            UIStateValueDescriptor d;
            d.label = m_repeat ? "Repeat" : "No Repeat";
            d.description = m_repeat ? "Repeat when playback reaches the end"
                                     : "No repeat when playback reaches the end";
            d.icon = m_repeat
                         ? ImageSet(wxART_REPEAT,
                                    Path(slv_core_pop, "interface-essential/button-loop.svg"))
                         : ImageSet(wxART_SEQUENCE,
                                    Path(slv_core_pop, "interface-essential/arrange-number.svg"));
            return d;
        })
        .initValue(UIStateVariant{false})
        .valueRef(&m_repeatStateSink)
        .connect([this](const UIStateVariant& nv, const UIStateVariant&) {
            if (const bool* b = std::get_if<bool>(&nv))
                m_repeat = *b;
        })
        .install();

    state(ID_STATE_AUDIO_SPECTRUM, "media/mediaplayer", "audio_spectrum", 11)
        .label("Spectrum view")
        .description("Show spectrum bars instead of waveform (audio)")
        .stateType(UIStateType::BOOL)
        .initValue(UIStateVariant{false})
        .valueRef(&m_spectrumStateSink)
        .connect([this](const UIStateVariant& nv, const UIStateVariant&) {
            if (const bool* b = std::get_if<bool>(&nv)) {
                m_audioSpectrum = *b;
                if (m_visualizer)
                    asViz(m_visualizer)->setSpectrumMode(m_audioSpectrum);
            }
        })
        .install();
}

MediaPlayerBody::~MediaPlayerBody() {
    if (m_vizTimer.IsRunning())
        m_vizTimer.Stop();
}

void MediaPlayerBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    m_frame = dynamic_cast<uiFrame*>(parent);
    if (!m_frame) {
        wxMessageBox("Parent is not a uiFrame", "Media Player", wxOK | wxICON_ERROR);
        return;
    }

    m_root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    m_visualizer = new MediaAudioVisualizer(m_root);
    asViz(m_visualizer)->setSpectrumMode(m_audioSpectrum);
    mainSizer->Add(m_visualizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);

#if HAVE_WX_MEDIA
    m_media = new wxMediaCtrl(m_root, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    mainSizer->Add(m_media, 1, wxEXPAND | wxALL, 4);

    m_media->Bind(wxEVT_MEDIA_FINISHED, &MediaPlayerBody::onMediaFinished, this);
#else
    mainSizer->Add(
        new wxStaticText(m_root, wxID_ANY,
                         _("wxMediaCtrl is not available (link wxWidgets media library).")),
        0, wxALL, 8);
#endif

    wxBitmap bmpPlay = ImageSet(wxART_GO_FORWARD, //
                                Path(slv_core_pop, "entertainment/button-play.svg"))
                           .toBitmap1(kControlIconPx, kControlIconPx);
    wxBitmap bmpPause = ImageSet(wxART_PAUSE, //
                                 Path(slv_core_pop, "entertainment/button-pause-2.svg"))
                            .toBitmap1(kControlIconPx, kControlIconPx);
    wxBitmap bmpStop = ImageSet(wxART_STOP, //
                                Path(slv_core_pop, "entertainment/button-stop.svg"))
                           .toBitmap1(kControlIconPx, kControlIconPx);

    auto* bar = new wxPanel(m_root, wxID_ANY);
    auto* barSizer = new wxBoxSizer(wxHORIZONTAL);
    barSizer->AddStretchSpacer();

    auto makeBtn = [this, bar, &barSizer](const wxBitmap& bmp, const wxString& tip,
                                          void (MediaPlayerBody::*fn)(PerformContext*)) {
        auto* b = new wxBitmapButton(bar, wxID_ANY, bmp, wxDefaultPosition, wxDefaultSize,
                                     wxBU_AUTODRAW | wxBORDER_NONE);
        b->SetToolTip(tip);
        b->Bind(wxEVT_BUTTON, [this, fn](wxCommandEvent& e) {
            auto pc = toPerformContext(e);
            (this->*fn)(&pc);
        });
        barSizer->Add(b, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
    };

    makeBtn(bmpPlay, _("Play"), &MediaPlayerBody::onPlay);
    makeBtn(bmpPause, _("Pause"), &MediaPlayerBody::onPause);
    makeBtn(bmpStop, _("Stop"), &MediaPlayerBody::onStop);

    barSizer->AddStretchSpacer();
    bar->SetSizer(barSizer);
    mainSizer->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    m_root->SetSizer(mainSizer);

    m_vizTimer.SetOwner(m_root);
    m_root->Bind(wxEVT_TIMER, &MediaPlayerBody::onVizTimer, this, m_vizTimer.GetId());
    m_frame->Bind(wxEVT_CLOSE_WINDOW, &MediaPlayerBody::onFrameClose, this);

    syncVisualizerLayout();
    updateFrameTitle();
}

wxEvtHandler* MediaPlayerBody::getEventHandler() {
    return m_root ? m_root->GetEventHandler() : nullptr;
}

wxLongLong MediaPlayerBody::playbackTellMs() const {
#if HAVE_WX_MEDIA
    if (m_media)
        return m_media->Tell();
#endif
    return wxLongLong(0);
}

bool MediaPlayerBody::isPlaybackPlaying() const {
#if HAVE_WX_MEDIA
    if (m_media)
        return m_media->GetState() == wxMEDIASTATE_PLAYING;
#endif
    return false;
}

void MediaPlayerBody::onFrameClose(wxCloseEvent& event) {
    if (m_vizTimer.IsRunning())
        m_vizTimer.Stop();
    event.Skip();
}

void MediaPlayerBody::onVizTimer(wxTimerEvent&) { refreshVisualizer(); }

void MediaPlayerBody::refreshVisualizer() {
    if (!m_visualizer)
        return;
    asViz(m_visualizer)->syncPlayback(isPlaybackPlaying(), playbackTellMs());
    m_visualizer->Refresh(false);
}

void MediaPlayerBody::syncVisualizerLayout() {
    if (!m_visualizer)
        return;
    const bool audio = m_file.has_value() && extensionIsAudio(m_file->getPath());
    m_visualizer->Show(audio);
    if (m_root)
        m_root->Layout();
}

void MediaPlayerBody::updateFrameTitle() {
    if (!m_frame)
        return;
    if (!m_file)
        m_frame->SetTitle("Media Player");
    else
        m_frame->SetTitle(wxString("Media Player — ") + displayNameFromPath(m_file->getPath()));
}

void MediaPlayerBody::loadVolumeFile(const VolumeFile& file, bool autoplay) {
    m_file = file;
    updateFrameTitle();
    syncVisualizerLayout();

#if HAVE_WX_MEDIA
    if (!m_media || !m_vm)
        return;
    ShellApp* sh = ShellApp::getInstance();
    if (!sh || !sh->vfsDaemon().isRunning()) {
        wxLogWarning("Media Player: VFS daemon is not running");
        return;
    }
    const int idx = volumeIndexFor(m_vm, file.getVolume());
    if (idx < 0) {
        wxLogWarning("Media Player: could not resolve volume");
        return;
    }
    std::string rel = stripVolumeSlashes(file.getPath());
    std::string url = sh->vfsDaemon().httpUrlForVolumePath(static_cast<size_t>(idx), rel);
    if (url.empty()) {
        wxLogWarning("Media Player: could not build URL");
        return;
    }
    wxURI u(wxString::FromUTF8(url.data(), static_cast<int>(url.size())));
    if (!m_media->Load(u)) {
        wxLogWarning("Media Player: Load failed");
        return;
    }
    if (autoplay)
        m_media->Play();
#else
    (void)autoplay;
#endif

    if (m_vizTimer.IsRunning())
        m_vizTimer.Stop();
#if HAVE_WX_MEDIA
    if (extensionIsAudio(file.getPath()))
        m_vizTimer.Start(kVizTimerMs);
#endif
    refreshVisualizer();
}

void MediaPlayerBody::openVolumePath(size_t volumeIndex, const std::string& pathOnVolume) {
    if (!m_vm || volumeIndex >= m_vm->getVolumeCount())
        return;
    Volume* v = m_vm->getVolume(volumeIndex);
    if (!v)
        return;
    std::string p = pathOnVolume;
    if (p.empty())
        p = "/";
    else if (p.front() != '/')
        p = "/" + p;
    loadVolumeFile(VolumeFile(v, p), true);
}

void MediaPlayerBody::onOpen(PerformContext*) {
    if (!m_frame || !m_vm) {
        wxMessageBox("No volume manager", "Media Player", wxOK | wxICON_ERROR);
        return;
    }
    wxString startDir = "/";
    if (m_file) {
        std::string path = m_file->getPath();
        size_t slash = path.rfind('/');
        if (slash != std::string::npos && slash > 0)
            startDir = utf8Wx(path.substr(0, slash));
        else if (slash == 0 && path.size() > 1)
            startDir = utf8Wx(path.substr(0, slash + 1));
    }

    ChooseFileDialog dlg(m_frame, m_vm, _("Open media"), FileDialogMode::Open, startDir, "");
    dlg.addFilter(
        _("Audio/Video"),
        "*.mp3;*.wav;*.flac;*.ogg;*.oga;*.opus;*.m4a;*.aac;*.wma;*.mp4;*.webm;*.mkv;*.avi;*.mov");
    dlg.addFilter(_("All files"), "*.*");
    dlg.setFileMustExist(true);
    if (dlg.ShowModal() != wxID_OK)
        return;
    auto vf = dlg.getVolumeFile();
    if (!vf)
        return;
    loadVolumeFile(*vf, true);
}

void MediaPlayerBody::onPlay(PerformContext*) {
#if HAVE_WX_MEDIA
    if (m_media)
        m_media->Play();
#endif
    if (m_file && extensionIsAudio(m_file->getPath()) && !m_vizTimer.IsRunning())
        m_vizTimer.Start(kVizTimerMs);
    refreshVisualizer();
}

void MediaPlayerBody::onPause(PerformContext*) {
#if HAVE_WX_MEDIA
    if (m_media)
        m_media->Pause();
#endif
    refreshVisualizer();
}

void MediaPlayerBody::onStop(PerformContext*) {
#if HAVE_WX_MEDIA
    if (m_media)
        m_media->Stop();
#endif
    refreshVisualizer();
}

#if HAVE_WX_MEDIA
void MediaPlayerBody::onMediaFinished(wxMediaEvent&) {
    if (m_repeat && m_media) {
        m_media->Seek(0);
        m_media->Play();
        return;
    }
    refreshVisualizer();
}
#endif

} // namespace os

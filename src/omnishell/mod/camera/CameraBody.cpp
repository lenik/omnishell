#include "CameraBody.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../shell/Shell.hpp"
#include "../../ui/Location.hpp"
#include "../../ui/ThemeStyles.hpp"
#include "../../ui/dialogs/ChooseFileDialog.hpp"
#include "../../ui/dialogs/ChooseFolderDialog.hpp"
#include "../../ui/widget/FileListView.hpp"
#include "../../wx/artprovs.hpp"
#include "../mediaplayer/MediaPlayerApp.hpp"
#include "../paint/PaintApp.hpp"
#include "../photoviewer/PhotoViewerApp.hpp"

#include <bas/volume/DirEntry.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/bitmap.h>
#include <wx/bmpbuttn.h>
#include <wx/dcbuffer.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/mstream.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/uri.h>

#if HAVE_WX_MEDIA
#include <wx/mediactrl.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ThemeStyles;

namespace os {

namespace {

bool isImageExt(std::string_view ext) {
    static const char* kImg[] = {"png", "jpg", "jpeg", "bmp", "gif", "webp"};
    for (const char* e : kImg) {
        if (ext == e)
            return true;
    }
    return false;
}

bool isVideoExt(std::string_view ext) {
    static const char* kVid[] = {"mp4",  "webm", "mkv", "avi", "mov", "mpg",
                                 "mpeg", "m4v",  "3gp", "flv", "wmv"};
    for (const char* e : kVid) {
        if (ext == e)
            return true;
    }
    return false;
}

int volumeIndexFor(VolumeManager* vm, Volume* vol) {
    if (!vm || !vol)
        return -1;
    for (size_t i = 0; i < vm->getVolumeCount(); ++i) {
        if (vm->getVolume(i) == vol)
            return static_cast<int>(i);
    }
    return -1;
}

std::string stripLeadingSlash(std::string s) {
    while (!s.empty() && s.front() == '/')
        s.erase(s.begin());
    return s;
}

enum {
    ID_GROUP_CAMERA = uiFrame::ID_APP_HIGHEST + 460,
    ID_CAPTURE = uiFrame::ID_APP_HIGHEST + 461,
    ID_RECORD = uiFrame::ID_APP_HIGHEST + 462,
    ID_PAUSE = uiFrame::ID_APP_HIGHEST + 463,
    ID_STOP = uiFrame::ID_APP_HIGHEST + 464,
    ID_REPLAY = uiFrame::ID_APP_HIGHEST + 465,
    ID_SETTINGS = uiFrame::ID_APP_HIGHEST + 466,
};

} // namespace

#ifndef wxART_CAMERA
#define wxART_CAMERA wxART_MAKE_ART_ID(wxART_CAMERA)
#endif

#ifndef wxART_PHOTO
#define wxART_PHOTO wxART_MAKE_ART_ID(wxART_PHOTO)
#endif

#ifndef wxART_VIDEO
#define wxART_VIDEO wxART_MAKE_ART_ID(wxART_VIDEO)
#endif

CameraBody::CameraBody() {
    const os::IconTheme* theme = os::app.getIconTheme();
    group(ID_GROUP_CAMERA, "media", "camera", 1000)
        .label("&Camera")
        .description("Camera controls")
        .icon(theme->icon("camera", "group.camera"))
        .install();

    int seq = 0;
    action(ID_CAPTURE, "media/camera", "capture", seq++)
        .label("&Capture")
        .description("Capture photo")
        .icon(theme->icon("camera", "media.capture"))
        .performFn([this](PerformContext*) {
            try {
                capturePhoto();
            } catch (const std::exception& ex) {
                wxString msg =
                    wxString::FromUTF8(ex.what(), static_cast<int>(std::strlen(ex.what())));
                if (m_videoStatus)
                    m_videoStatus->SetLabel("Capture failed: " + msg);
            } catch (...) {
                if (m_videoStatus)
                    m_videoStatus->SetLabel("Capture failed: unknown exception.");
            }
        })
        .install();

    action(ID_RECORD, "media/camera", "record", seq++)
        .label("&Record")
        .description("Record video")
        .icon(theme->icon("camera", "media.record"))
        .performFn([this](PerformContext*) {
            try {
                recordVideo();
            } catch (...) {
                if (m_videoStatus)
                    m_videoStatus->SetLabel("Record failed.");
            }
        })
        .install();

    action(ID_PAUSE, "media/camera", "pause", seq++)
        .label("&Pause")
        .description("Pause or resume")
        .icon(theme->icon("camera", "media.pause"))
        .performFn([this](PerformContext*) {
            try {
                pauseOrReplayVideo();
            } catch (...) {
                if (m_videoStatus)
                    m_videoStatus->SetLabel("Pause/Resume failed.");
            }
        })
        .install();

    action(ID_STOP, "media/camera", "stop", seq++)
        .label("&Stop")
        .description("Stop video playback")
        .icon(theme->icon("camera", "media.stop"))
        .performFn([this](PerformContext*) {
            try {
                stopVideo();
            } catch (...) {
                if (m_videoStatus)
                    m_videoStatus->SetLabel("Stop failed.");
            }
        })
        .install();

    action(ID_REPLAY, "media/camera", "replay", seq++)
        .label("&Replay")
        .description("Replay video from the start")
        .icon(theme->icon("camera", "media.replay"))
        .performFn([this](PerformContext*) {
            try {
                replayVideo();
            } catch (...) {
                if (m_videoStatus)
                    m_videoStatus->SetLabel("Replay failed.");
            }
        })
        .install();

    action(ID_SETTINGS, "media/camera", "settings", seq++)
        .label("&Settings")
        .description("Camera settings")
        .icon(theme->icon("camera", "app.settings"))
        .performFn([this](PerformContext*) {
            try {
                onSettings();
            } catch (...) {
                if (m_videoStatus)
                    m_videoStatus->SetLabel("Open settings failed.");
            }
        })
        .install();
}

CameraBody::~CameraBody() {
#if OMNISHELL_HAVE_GSTREAMER
    m_previewActive = false;
    if (m_previewTimer) {
        m_previewTimer->Stop();
        if (m_root)
            m_root->Unbind(wxEVT_TIMER, &CameraBody::onPreviewTick, this, m_previewTimer->GetId());
        delete m_previewTimer;
        m_previewTimer = nullptr;
    }
#endif
    m_gstPreview.reset();
}

void CameraBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    m_frame = dynamic_cast<uiFrame*>(parent);
    if (!m_frame)
        return;
    // Most modules just read VolumeManager from the global ShellApp.
    if (auto* sh = ShellApp::getInstance())
        m_vm = sh->getVolumeManager();

    m_root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    m_root->SetMinSize(wxSize(720, 520));

    auto* outer = new wxBoxSizer(wxVERTICAL);

    auto* topBar = new wxBoxSizer(wxHORIZONTAL);
    wxArrayString choices;
    choices.Add("Photo");
    choices.Add("Video");
    m_modeBox = new wxRadioBox(m_root, wxID_ANY, "Mode", wxDefaultPosition, wxDefaultSize, choices,
                               1, wxRA_SPECIFY_ROWS);
    m_modeBox->SetSelection(0);
    m_modeBox->Bind(wxEVT_RADIOBOX, &CameraBody::onModeChanged, this);

    topBar->Add(m_modeBox, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
    topBar->AddStretchSpacer();

    outer->Add(topBar, 0, wxEXPAND);

    // Preview area.
    auto* preview = new wxPanel(m_root, wxID_ANY);
    auto* previewSizer = new wxBoxSizer(wxVERTICAL);

    m_photoPreview =
        new wxStaticBitmap(preview, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize(-1, 330),
                           wxBORDER_SUNKEN | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    previewSizer->Add(m_photoPreview, 1, wxEXPAND | wxALL, 8);

    m_videoStatus = new wxStaticText(preview, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxALIGN_CENTRE_HORIZONTAL);
    previewSizer->Add(m_videoStatus, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

#if HAVE_WX_MEDIA
    m_media =
        new wxMediaCtrl(preview, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    previewSizer->Add(m_media, 1, wxEXPAND | wxALL, 8);
    m_media->Bind(wxEVT_MEDIA_FINISHED, [this](wxMediaEvent&) {
        // Treat finish like stop; user can replay.
        m_media->Stop();
        // keep replay enabled (handled by refreshModeUi()).
        m_videoLoaded = true;
        refreshModeUi();
    });
    m_media->Show(false);
#else
    (void)previewSizer;
#endif

    preview->SetSizer(previewSizer);
    outer->Add(preview, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Gallery bar.
    auto* galleryHolder = new wxPanel(m_root, wxID_ANY);
    auto* gallerySizer = new wxBoxSizer(wxVERTICAL);

    // Destination dir setup must happen before we can create FileListView.
    ensureDestinationDir();

    if (m_destDir) {
        Location loc(m_destDir->getVolume(), m_destDir->getPath());
        m_gallery = new FileListView(galleryHolder, loc);
        m_gallery->setViewMode("grid");
        m_gallery->SetBackgroundColour(*wxBLACK);
        m_gallery->SetForegroundColour(*wxWHITE);
        m_gallery->setThumbnailMaxBytes(2 * 1024 * 1024); // allow real photo thumbnails
        m_gallery->enableRecencyFade(kMaxRecent, 8, true);

        // Filter only the media type for current mode (photo/video).
        m_gallery->setEntryFilter([this](const DirEntry& e) -> bool {
            if (!e.isRegularFile())
                return false;
            const std::string ext = lowerExtOf(e.name);
            if (m_mode == CameraMode::Photo)
                return isImageExt(ext);
            return isVideoExt(ext);
        });

        m_gallery->onFileActivated([this](VolumeFile& f) { onOpenGalleryItem(f); });
        // Ensure camera shortcuts work even when focus is inside the gallery.
        m_gallery->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& ev) { //
            OnKeyDown(ev);
        });
        m_gallery->SetMinSize(wxSize(-1, 120));
        m_gallery->Refresh();
        gallerySizer->Add(m_gallery, 1, wxEXPAND);
    }

    galleryHolder->SetSizer(gallerySizer);
    outer->Add(galleryHolder, 0, wxEXPAND | wxALL, 8);

    m_root->SetSizer(outer);

    scanRecent();
    applyGalleryFilterAndRefresh();
    refreshModeUi();

    // Keyboard shortcuts live on the frame so they work even when focus is inside the gallery
    // widget.
    if (m_frame)
        m_frame->Bind(wxEVT_KEY_DOWN, &CameraBody::OnKeyDown, this);
    if (m_root)
        m_root->SetFocus();

#if OMNISHELL_HAVE_GSTREAMER
    m_root->CallAfter([this]() {
        if (!m_photoPreview || !m_root)
            return;
        wxSize sz = m_photoPreview->GetClientSize();
        const int pw = std::max(320, sz.GetWidth() > 0 ? sz.GetWidth() : 640);
        const int ph = std::max(240, sz.GetHeight() > 0 ? sz.GetHeight() : 480);
        m_gstPreview = std::make_unique<CameraGstPreview>();
        if (m_previewPlaying) {
            if (!m_gstPreview->start("/dev/video0", pw, ph)) {
                m_gstPreview.reset();
                if (m_videoStatus)
                    m_videoStatus->SetLabel(
                        "Camera preview unavailable (check /dev/video0 and GStreamer)");
                return;
            }
        }
        m_previewTimer = new wxTimer(m_root, wxID_ANY);
        m_root->Bind(wxEVT_TIMER, &CameraBody::onPreviewTick, this, m_previewTimer->GetId());
        if (m_previewPlaying)
            m_previewTimer->Start(33);
        refreshModeUi();
    });
#endif
}

wxEvtHandler* CameraBody::getEventHandler() { return m_root ? m_root->GetEventHandler() : nullptr; }

void CameraBody::onModeChanged(wxCommandEvent& e) {
    (void)e;
    const CameraMode newMode =
        (m_modeBox && m_modeBox->GetSelection() == 1) ? CameraMode::Video : CameraMode::Photo;

#if HAVE_WX_MEDIA
    if (m_mode == CameraMode::Video && newMode == CameraMode::Photo && m_media) {
        m_media->Stop();
        m_videoLoaded = true;
    }
#endif

    m_mode = newMode;
    refreshModeUi();
    applyGalleryFilterAndRefresh();

    if (m_gallery) {
        m_gallery->clearSelection();
        const auto& order = (m_mode == CameraMode::Photo) ? m_recentPhotoOrder : m_recentVideoOrder;
        if (!order.empty())
            m_gallery->selectFile(order.back());
    }
}

void CameraBody::refreshModeUi() {
    const bool isPhoto = (m_mode == CameraMode::Photo);
    (void)isPhoto;

#if HAVE_WX_MEDIA
    if (m_media)
        m_media->Show(m_mode == CameraMode::Video && m_videoLoaded);
#endif
    if (m_photoPreview)
        m_photoPreview->Show(true);

    if (m_videoStatus) {
        const wxString pv = m_previewPlaying ? "playing" : "still";
        if (m_mode == CameraMode::Video) {
#if HAVE_WX_MEDIA
            if (m_videoLoaded && m_media) {
                const auto st = m_media->GetState();
                const wxString ms = (st == wxMEDIASTATE_PLAYING)  ? "playing"
                                    : (st == wxMEDIASTATE_PAUSED) ? "paused"
                                                                  : "stopped";
                m_videoStatus->SetLabel("Video mode — media " + ms + "; preview " + pv +
                                        " preview");
            } else {
                m_videoStatus->SetLabel("Video mode — record via file selection (preview " + pv +
                                        " preview)");
            }
#else
            m_videoStatus->SetLabel("Video mode");
#endif
        } else {
            m_videoStatus->SetLabel("Photo mode — capture via file selection (preview " + pv +
                                    " preview)");
        }
    }

    if (m_root)
        m_root->Layout();
}

void CameraBody::ensureDestinationDir() {
    if (m_destDir)
        return;
    if (!m_vm) {
        if (auto* sh = ShellApp::getInstance())
            m_vm = sh->getVolumeManager();
    }
    if (!m_vm)
        return;

    Volume* vol = m_vm->getDefaultVolume();
    if (!vol)
        return;

    // DCF: store captures under /DCIM/<subdir> (e.g. 100OMNI).
    m_destDir = VolumeFile(vol, "/DCIM/100OMNI");
    try {
        m_destDir->createDirectories();
    } catch (...) {
        // ignore; UI will just show empty gallery.
    }
}

int CameraBody::nextDcfCounterForPrefix(const std::string& prefix) const {
    if (!m_destDir)
        return 1;

    std::vector<std::unique_ptr<FileStatus>> entries;
    try {
        entries = m_destDir->getVolume()->readDir(m_destDir->getPath(), false);
    } catch (...) {
        return 1;
    }
    std::unordered_set<int> used;
    int maxSeen = 0;

    for (const auto& fs : entries) {
        if (!fs)
            continue;
        const DirEntry& de = *fs;
        if (!de.isRegularFile())
            continue;

        if (de.name.size() < prefix.size() + 4)
            continue;
        if (de.name.rfind(prefix, 0) != 0)
            continue;

        // prefix + 4 digits
        bool ok = true;
        int val = 0;
        for (size_t i = 0; i < 4; ++i) {
            char c = de.name[prefix.size() + i];
            if (c < '0' || c > '9') {
                ok = false;
                break;
            }
            val = val * 10 + (c - '0');
        }
        if (!ok)
            continue;

        used.insert(val);
        maxSeen = std::max(maxSeen, val);
    }

    int cand = (maxSeen >= 9999) ? 1 : (maxSeen + 1);
    for (int i = 0; i < 10000; ++i) {
        if (cand < 1 || cand > 9999)
            cand = (cand < 1) ? 1 : 9999;
        if (used.find(cand) == used.end())
            return cand;
        ++cand;
        if (cand > 9999)
            cand = 1;
    }
    return 1;
}

std::string CameraBody::dcfFilename(const std::string& prefix, int counter,
                                    const std::string& extUpper) const {
    wxString digits = wxString::Format("%04d", counter);
    wxString out = wxString(prefix) + digits;
    if (!extUpper.empty())
        out += "." + wxString(extUpper);
    return std::string(out.mb_str());
}

std::string CameraBody::dcfPhotoExtUpper(const std::string& srcExtLower) const {
    if (srcExtLower == "jpg" || srcExtLower == "jpeg" || srcExtLower == "JPG" ||
        srcExtLower == "JPEG")
        return "JPG";
    if (srcExtLower.empty())
        return "JPG";
    std::string u = srcExtLower;
    for (char& c : u)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return u;
}

std::string CameraBody::dcfVideoExtUpper(const std::string& srcExtLower) const {
    if (srcExtLower == "mp4" || srcExtLower == "MP4")
        return "MP4";
    if (srcExtLower.empty())
        return "MP4";
    std::string u = srcExtLower;
    for (char& c : u)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return u;
}

wxString CameraBody::nowStamp() const {
    wxDateTime t = wxDateTime::Now();
    // Include milliseconds to avoid collisions when user captures multiple times quickly.
    return t.Format("%Y%m%d_%H%M%S") + wxString::Format("_%03d", t.GetMillisecond());
}

wxSize CameraBody::fitToBoxMaintainAspect(int srcW, int srcH, const wxSize& box) const {
    if (srcW <= 0 || srcH <= 0 || box.x <= 1 || box.y <= 1)
        return wxSize(std::max(1, srcW), std::max(1, srcH));

    const double sx = static_cast<double>(box.x) / static_cast<double>(srcW);
    const double sy = static_cast<double>(box.y) / static_cast<double>(srcH);
    const double s = std::min(sx, sy);
    const int tw = std::max(1, static_cast<int>(srcW * s));
    const int th = std::max(1, static_cast<int>(srcH * s));
    return wxSize(tw, th);
}

void CameraBody::togglePreviewPlaying() {
    m_previewPlaying = !m_previewPlaying;

#if OMNISHELL_HAVE_GSTREAMER
    constexpr int kPreviewTickMs = 33;
    if (!m_previewPlaying) {
        if (m_previewTimer)
            m_previewTimer->Stop();
        if (m_gstPreview)
            m_gstPreview->stop();
    } else {
        // Resuming: lazily create preview timer/pipeline if they weren't initialized.
        if (!m_previewTimer && m_root) {
            m_previewTimer = new wxTimer(m_root, wxID_ANY);
            m_root->Bind(wxEVT_TIMER, &CameraBody::onPreviewTick, this, m_previewTimer->GetId());
        }
        if (!m_gstPreview)
            m_gstPreview = std::make_unique<CameraGstPreview>();

        if (m_gstPreview && !m_gstPreview->isRunning() && m_photoPreview) {
            wxSize sz = m_photoPreview->GetClientSize();
            const int pw = std::max(320, sz.x > 0 ? sz.x : 640);
            const int ph = std::max(240, sz.y > 0 ? sz.y : 480);
            if (!m_gstPreview->start("/dev/video0", pw, ph)) {
                if (m_videoStatus)
                    m_videoStatus->SetLabel(
                        "Camera preview unavailable (check /dev/video0 and GStreamer)");
            }
        }
        if (m_previewTimer)
            m_previewTimer->Start(kPreviewTickMs);
    }
#endif
    refreshModeUi();
}

void CameraBody::deleteLastCapture() {
    if (!m_lastSavedFile)
        return;

    try {
        if (m_lastSavedFile->isFile())
            m_lastSavedFile->removeFile();
    } catch (...) {
    }

    m_lastSavedFile.reset();

#if HAVE_WX_MEDIA
    if (m_mode == CameraMode::Video && m_videoLoaded && m_media) {
        m_media->Stop();
        m_videoLoaded = false;
    }
#endif

    applyGalleryFilterAndRefresh();

    if (m_gallery) {
        m_gallery->clearSelection();
        const auto& order = (m_mode == CameraMode::Photo) ? m_recentPhotoOrder : m_recentVideoOrder;
        if (!order.empty())
            m_gallery->selectFile(order.back());
    }

    refreshModeUi();
}

void CameraBody::navigateGallery(int delta) {
    if (!m_gallery)
        return;

    const auto& order = (m_mode == CameraMode::Photo) ? m_recentPhotoOrder : m_recentVideoOrder;
    if (order.empty())
        return;

    auto sel = m_gallery->getSelectedFiles();
    size_t idx = 0;
    if (!sel.empty()) {
        auto it = std::find(order.begin(), order.end(), sel.front());
        idx = (it != order.end()) ? static_cast<size_t>(std::distance(order.begin(), it)) : 0;
    } else {
        idx = (delta < 0) ? (order.size() ? order.size() - 1 : 0) : 0;
    }

    if (delta < 0) {
        if (idx == 0)
            return;
        idx--;
    } else {
        if (idx + 1 >= order.size())
            return;
        idx++;
    }

    m_gallery->clearSelection();
    m_gallery->selectFile(order[idx]);
}

void CameraBody::OnKeyDown(wxKeyEvent& event) {
    if (event.ControlDown() || event.AltDown() || event.MetaDown()) {
        event.Skip();
        return;
    }

    const int k = event.GetKeyCode();

    if (k == WXK_LEFT) {
        navigateGallery(-1);
        event.Skip(false);
        return;
    }
    if (k == WXK_RIGHT) {
        navigateGallery(+1);
        event.Skip(false);
        return;
    }
    if (k == WXK_BACK) {
        deleteLastCapture();
        event.Skip(false);
        return;
    }
    if (k == WXK_F2) {
        togglePreviewPlaying();
        event.Skip(false);
        return;
    }

    if (k == WXK_SPACE) {
        if (m_mode == CameraMode::Photo) {
            capturePhoto();
        } else {
#if HAVE_WX_MEDIA
            pauseOrReplayVideo();
#else
            recordVideo();
#endif
        }
        event.Skip(false);
        return;
    }

    if (k == WXK_RETURN || k == WXK_NUMPAD_ENTER) {
        if (m_mode == CameraMode::Photo) {
            capturePhoto();
        } else {
#if HAVE_WX_MEDIA
            if (!m_videoLoaded)
                recordVideo();
            else
                stopVideo();
#else
            recordVideo();
#endif
        }
        event.Skip(false);
        return;
    }

    event.Skip();
}

std::string CameraBody::lowerExtOf(const std::string& path) const {
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= path.size())
        return {};
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}

bool CameraBody::extMatches(const std::string& ext) const { // currently unused
    if (m_mode == CameraMode::Photo)
        return isImageExt(ext);
    return isVideoExt(ext);
}

void CameraBody::scanRecent() {
    m_recentPhotoNames.clear();
    m_recentVideoNames.clear();
    m_recentPhotoOrder.clear();
    m_recentVideoOrder.clear();

    if (!m_destDir)
        return;

    auto entries = m_destDir->getVolume()->readDir(m_destDir->getPath(), false);
    std::vector<DirEntry> photos;
    std::vector<DirEntry> videos;

    for (const auto& fs : entries) {
        if (!fs)
            continue;
        const DirEntry& de = *fs;
        if (!de.isRegularFile())
            continue;

        const std::string ext = lowerExtOf(de.name);
        if (isImageExt(ext)) {
            photos.push_back(de);
        } else if (isVideoExt(ext)) {
            videos.push_back(de);
        }
    }

    // Keep full ordering (oldest -> newest) so gallery "new captured appended to the right".
    auto sortAsc = [](const DirEntry& a, const DirEntry& b) { return a.modifiedTime < b.modifiedTime; };
    std::sort(photos.begin(), photos.end(), sortAsc);
    std::sort(videos.begin(), videos.end(), sortAsc);

    m_recentPhotoOrder.reserve(photos.size());
    for (const auto& p : photos) {
        m_recentPhotoOrder.push_back(p.name);
    }
    m_recentVideoOrder.reserve(videos.size());
    for (const auto& v : videos) {
        m_recentVideoOrder.push_back(v.name);
    }

    // "Recent" set (newest K) for other features that rely on name membership.
    const size_t photoStart = photos.size() > kMaxRecent ? photos.size() - kMaxRecent : 0;
    for (size_t i = photoStart; i < photos.size(); ++i)
        m_recentPhotoNames.insert(photos[i].name);
    const size_t videoStart = videos.size() > kMaxRecent ? videos.size() - kMaxRecent : 0;
    for (size_t i = videoStart; i < videos.size(); ++i)
        m_recentVideoNames.insert(videos[i].name);
}

void CameraBody::applyGalleryFilterAndRefresh() {
    if (!m_gallery)
        return;
    scanRecent();
    m_gallery->refresh();
    // Oldest -> newest, so newly captured items appear on the right in grid view.
    m_gallery->sortByDate(true);

    // Select newest item by default so LEFT/RIGHT navigation starts from the latest.
    const auto& order = (m_mode == CameraMode::Photo) ? m_recentPhotoOrder : m_recentVideoOrder;
    if (!order.empty()) {
        m_gallery->clearSelection();
        m_gallery->selectFile(order.back());
    }
}

void CameraBody::capturePhoto() {
    if (m_mode != CameraMode::Photo)
        return;
    if (!m_vm) {
        if (m_videoStatus)
            m_videoStatus->SetLabel("Capture failed: VolumeManager unavailable.");
        return;
    }
    if (!m_destDir)
        ensureDestinationDir();
    if (!m_destDir) {
        if (m_videoStatus)
            m_videoStatus->SetLabel("Capture failed: destination folder unavailable.");
        return;
    }
    // If the destination dir is not actually created/accessible, avoid attempting capture.
    try {
        if (!m_destDir->isDirectory()) {
            if (m_videoStatus)
                m_videoStatus->SetLabel("Capture failed: DCIM destination is not a directory.");
            return;
        }
    } catch (...) {
        // If isDirectory throws, fall through to the write attempt (it will be handled below).
    }

    const std::string prefix = "DSC_";
    std::vector<uint8_t> rgb = m_lastPreviewRgb;
    int frameW = m_lastPreviewW;
    int frameH = m_lastPreviewH;

#if OMNISHELL_HAVE_GSTREAMER
    // If cache is empty/stale, try to pull one frame synchronously.
    if ((frameW <= 0 || frameH <= 0 || rgb.empty()) && m_gstPreview) {
        if (!m_gstPreview->isRunning() && m_photoPreview) {
            const wxSize box = m_photoPreview->GetClientSize();
            const int pw = std::max(320, box.x > 0 ? box.x : 640);
            const int ph = std::max(240, box.y > 0 ? box.y : 480);
            (void)m_gstPreview->start("/dev/video0", pw, ph);
        }
        for (int i = 0; i < 5; ++i) {
            if (m_gstPreview->tryPullRgbFrame(rgb, frameW, frameH) && frameW > 0 && frameH > 0 &&
                !rgb.empty())
                break;
        }
    }
#endif

    if (frameW <= 0 || frameH <= 0 || rgb.empty()) {
        if (m_videoStatus)
            m_videoStatus->SetLabel(
                "No camera frame available yet. Wait for preview, then capture.");
        return;
    }

    const size_t need = static_cast<size_t>(frameW) * static_cast<size_t>(frameH) * 3u;
    if (rgb.size() < need) {
        if (m_videoStatus)
            m_videoStatus->SetLabel("Capture failed: invalid preview frame size.");
        return;
    }

    wxImage img(frameW, frameH);
    unsigned char* rgbPtr = img.GetData();
    if (!rgbPtr) {
        if (m_videoStatus)
            m_videoStatus->SetLabel("Capture failed: wxImage data buffer unavailable.");
        return;
    }
    std::memcpy(rgbPtr, rgb.data(), need);

    // Encode to JPEG bytes in-memory (no tmpfile).
    wxMemoryOutputStream stream;
    if (!img.SaveFile(stream, wxBITMAP_TYPE_JPEG)) {
        if (m_videoStatus)
            m_videoStatus->SetLabel("Capture failed: JPEG encode failed.");
        return;
    }
    const size_t dataSize = static_cast<size_t>(stream.GetLength());
    if (dataSize == 0) {
        if (m_videoStatus)
            m_videoStatus->SetLabel("Capture failed: JPEG encode produced empty data.");
        return;
    }
    std::vector<uint8_t> jpegBytes(dataSize);
    if (stream.CopyTo(jpegBytes.data(), dataSize) != dataSize) {
        if (m_videoStatus)
            m_videoStatus->SetLabel("Capture failed: failed to copy encoded JPEG bytes.");
        return;
    }

    const int counter = nextDcfCounterForPrefix(prefix);
    const std::string savedName = dcfFilename(prefix, counter, "JPG");

    auto dest = m_destDir->resolve(savedName);
    dest->createParentDirectories();
    dest->writeFile(jpegBytes);
    m_lastSavedFile = *dest;

    scanRecent();
    applyGalleryFilterAndRefresh();

    if (m_gallery) {
        m_gallery->clearSelection();
        m_gallery->selectFile(savedName);
    }

    refreshModeUi();
    if (m_videoStatus)
        m_videoStatus->SetLabel("Saved photo: " +
                                wxString::FromUTF8(savedName.data(), savedName.size()));
}

void CameraBody::recordVideo() {
    if (m_mode != CameraMode::Video)
        return;
#if HAVE_WX_MEDIA
    if (!m_vm || !m_destDir)
        return;

    ChooseFileDialog dlg(m_frame ? static_cast<wxWindow*>(m_frame) : nullptr, m_vm, "Select Video",
                         FileDialogMode::Open, m_destDir->getPath(), "");
    dlg.addFilter("Videos", "*.mp4;*.webm;*.mkv;*.avi;*.mov;*.mpg;*.mpeg;*.m4v;*.3gp;*.flv;*.wmv");
    dlg.setFileMustExist(true);
    dlg.setMultiSelect(false);
    if (dlg.ShowModal() != wxID_OK)
        return;

    const auto files = dlg.getVolumeFiles();
    if (files.empty())
        return;
    const VolumeFile src = files.front();
    const std::string srcExtLower = lowerExtOf(src.getPath());
    const std::string extUpper = dcfVideoExtUpper(srcExtLower);
    const std::string prefix = "MVI_";
    const int counter = nextDcfCounterForPrefix(prefix);
    const std::string savedName = dcfFilename(prefix, counter, extUpper);

    const std::string destPath = m_destDir->getPath() + "/" + savedName;
    VolumeFile dest(m_destDir->getVolume(), destPath);
    dest.createParentDirectories();

    const auto data = src.readFile();
    dest.writeFile(data);
    m_lastSavedFile = dest;

    scanRecent();
    applyGalleryFilterAndRefresh();

    if (m_gallery) {
        m_gallery->clearSelection();
        m_gallery->selectFile(savedName);
    }

    // Load/play through wxMediaCtrl using VFS HTTP URL.
    if (!m_media)
        return;
    ShellApp* sh = ShellApp::getInstance();
    if (!sh || !sh->vfsDaemon().isRunning())
        return;

    Volume* dv = m_destDir->getVolume();
    const int idx = volumeIndexFor(m_vm, dv);
    if (idx < 0)
        return;

    std::string rel = stripLeadingSlash(dest.getPath());
    const std::string url = sh->vfsDaemon().httpUrlForVolumePath(static_cast<size_t>(idx), rel);
    if (url.empty())
        return;

    wxURI u(wxString::FromUTF8(url.data(), static_cast<int>(url.size())));
    if (!m_media->Load(u))
        return;
    m_media->Play();
    m_videoLoaded = true;
    refreshModeUi();
    if (m_videoStatus)
        m_videoStatus->SetLabel("Saved video: " +
                                wxString::FromUTF8(savedName.data(), savedName.size()));
#else
    (void)m_frame;
#endif
}

void CameraBody::pauseOrReplayVideo() {
#if HAVE_WX_MEDIA
    if (m_mode != CameraMode::Video)
        return;
    if (!m_videoLoaded) {
        // SPACE / Pause/Resume should also work during the "recording" stage
        // (i.e. when no video has been selected/loaded yet).
        recordVideo();
        return;
    }
    if (!m_media)
        return;
    const auto st = m_media->GetState();
    if (st == wxMEDIASTATE_STOPPED) {
        replayVideo();
        return;
    }
    if (st == wxMEDIASTATE_PLAYING)
        m_media->Pause();
    else if (st == wxMEDIASTATE_PAUSED)
        m_media->Play();
    refreshModeUi();
#endif
}

void CameraBody::stopVideo() {
#if HAVE_WX_MEDIA
    if (m_mode != CameraMode::Video || !m_videoLoaded)
        return;
    if (!m_media)
        return;
    m_media->Stop();
    m_videoLoaded = true;
    refreshModeUi();
#endif
}

void CameraBody::replayVideo() {
#if HAVE_WX_MEDIA
    if (m_mode != CameraMode::Video || !m_videoLoaded)
        return;
    if (!m_media)
        return;
    if (m_media->GetState() == wxMEDIASTATE_STOPPED || m_media->GetState() == wxMEDIASTATE_PAUSED ||
        m_media->GetState() == wxMEDIASTATE_PLAYING) {
        m_media->Seek(0);
        m_media->Play();
    }
    refreshModeUi();
#endif
}

void CameraBody::onOpenGalleryItem(VolumeFile& file) {
    if (!m_vm)
        return;
    const std::string ext = lowerExtOf(file.getPath());
    ShellApp* sh = ShellApp::getInstance();

    if (isImageExt(ext)) {
        ProcessPtr p = PhotoViewerApp::openImage(m_vm, file);
        if (p && sh && sh->getTaskbar())
            sh->getTaskbar()->addProcess(p);
        return;
    }
    if (isVideoExt(ext)) {
        ProcessPtr p = MediaPlayerApp::open(m_vm, file);
        if (p && sh && sh->getTaskbar())
            sh->getTaskbar()->addProcess(p);
        return;
    }

    // Fallback: just try media player.
    ProcessPtr p = MediaPlayerApp::open(m_vm, file);
    if (p && sh && sh->getTaskbar())
        sh->getTaskbar()->addProcess(p);
}

void CameraBody::onPreviewTick(wxTimerEvent& e) {
    (void)e;
#if OMNISHELL_HAVE_GSTREAMER
    if (!m_previewActive || !m_previewPlaying || !m_gstPreview || !m_photoPreview || !m_root)
        return;
    std::vector<uint8_t> rgb;
    int w = 0;
    int h = 0;
    if (!m_gstPreview->tryPullRgbFrame(rgb, w, h) || w <= 0 || h <= 0)
        return;

    constexpr int kMaxDim = 4096;
    if (w > kMaxDim || h > kMaxDim)
        return;
    const size_t need = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    if (need / (static_cast<size_t>(w) * 3u) != static_cast<size_t>(h))
        return; // overflow
    if (rgb.size() < need)
        return;

    wxImage img(w, h);
    if (!img.IsOk())
        return;
    unsigned char* rgbPtr = img.GetData();
    if (!rgbPtr)
        return;
    std::memcpy(rgbPtr, rgb.data(), need);

    // Keep latest RGB frame for instant capture action.
    m_lastPreviewW = w;
    m_lastPreviewH = h;
    m_lastPreviewRgb = rgb;

    wxBitmap bmp(img);
    const wxSize box = m_photoPreview->GetClientSize();
    if (box.x <= 2 || box.y <= 2) {
        m_photoPreview->SetBitmap(bmp);
        return;
    }

    // Render a fixed-size canvas so the preview doesn't "jump" due to bitmap sizing.
    const wxSize scaledDst = fitToBoxMaintainAspect(w, h, box);
    wxImage scaled = img.Scale(scaledDst.x, scaledDst.y, wxIMAGE_QUALITY_NORMAL);
    if (!scaled.IsOk()) {
        m_photoPreview->SetBitmap(bmp);
        return;
    }

    wxBitmap canvas(box.x, box.y);
    wxMemoryDC dc(canvas);
    dc.SetBackground(wxBrush(*wxBLACK));
    dc.Clear();
    const int ox = (box.x - scaledDst.x) / 2;
    const int oy = (box.y - scaledDst.y) / 2;
    dc.DrawBitmap(wxBitmap(scaled), ox, oy, true);
    dc.SelectObject(wxNullBitmap);
    m_photoPreview->SetBitmap(canvas);
#endif
}

void CameraBody::onSettings() {
    if (!m_vm || !m_frame)
        return;
    wxString cur = m_destDir ? wxString(m_destDir->getPath()) : wxString("/DCIM");
    ChooseFolderDialog dlg(m_frame, m_vm, "Camera Settings", "Choose where to save captures", cur);
    dlg.setShowNewFolderButton(true);
    dlg.setMustExist(false);
    if (dlg.ShowModal() != wxID_OK)
        return;
    auto opt = dlg.getVolumeFile();
    if (!opt)
        return;

    // DCF standard: save into a fixed subdirectory (e.g. 100OMNI) under the chosen base.
    constexpr const char* kDcfSubdir = "100OMNI";
    std::string base = opt->getPath();
    if (base.empty())
        base = "/";
    while (!base.empty() && base.back() == '/')
        base.pop_back();
    const size_t slash = base.rfind('/');
    const std::string lastSeg = (slash == std::string::npos) ? base : base.substr(slash + 1);
    if (lastSeg == kDcfSubdir) {
        m_destDir = *opt;
    } else {
        std::string full = base;
        if (full == "/")
            full.clear();
        full += "/";
        full += kDcfSubdir;
        m_destDir = VolumeFile(opt->getVolume(), full);
    }
    try {
        m_destDir->createDirectories();
    } catch (...) {
    }
    scanRecent();
    applyGalleryFilterAndRefresh();
    refreshModeUi();
}

} // namespace os

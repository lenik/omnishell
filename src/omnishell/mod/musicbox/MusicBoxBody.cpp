#include "MusicBoxBody.hpp"

#include "../../core/App.hpp"
#include "../../core/registry/RegistryService.hpp"
#include "../../shell/Shell.hpp"
#include "../../ui/ThemeStyles.hpp"
#include "../../ui/dialogs/ChooseFileDialog.hpp"
#include "../../ui/dialogs/ChooseFolderDialog.hpp"
#include "../../ui/widget/FileListView.hpp"
#include "../../wx/artprovs.hpp"

#include <bas/volume/DirEntry.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/gauge.h>
#include <wx/listctrl.h>
#include <wx/mediactrl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/timer.h>

#include <algorithm>
#include <cctype>
#include <random>
#include <sstream>

using namespace ThemeStyles;

namespace os {

namespace {

constexpr int kSeekTimerMs = 250;
constexpr int kGaugeRange = 1000;
constexpr const char* kPlaylistKey = "OmniShell.MusicBox.playlist";
constexpr const char* kPlaylistIndexKey = "OmniShell.MusicBox.playlistIndex";

static std::string lowerExtOf(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= path.size())
        return {};
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

static bool isAudioExt(const std::string& ext) {
    static const std::vector<std::string> kAudio = {"mp3", "wav", "flac", "ogg", "oga",  "opus",
                                                    "m4a", "aac", "wma",  "mid", "midi", "aiff",
                                                    "aif", "mpc", "ape",  "wv",  "dsf",  "dff"};
    return std::find(kAudio.begin(), kAudio.end(), ext) != kAudio.end();
}

static std::string stripLeadingSlash(std::string s) {
    while (!s.empty() && s.front() == '/')
        s.erase(s.begin());
    return s;
}

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

enum {
    ID_GROUP_MEDIA = uiFrame::ID_APP_HIGHEST + 1,
    ID_GROUP_PLAYLIST,
    ID_IMPORT_FILES,
    ID_IMPORT_DIR,
    ID_CLEAR_PLAYLIST,
    ID_REWIND,
    ID_FORWARD,
    ID_PLAY_PAUSE,
    ID_STOP,
    ID_REPEAT,
    ID_SHUFFLE,
};
MusicBoxBody::MusicBoxBody(VolumeManager* vm) : m_vm(vm) {
    const os::IconTheme* theme = os::app.getIconTheme();

    // Ensure the intermediate "media" menu exists and has a label.
    int seq = 100;
    group(ID_GROUP_MEDIA, "", "media", seq++)
        .label("&Media")
        .description("Media applications")
        .icon(theme->icon("musicbox", "group.media"))
        .install();
    group(ID_GROUP_PLAYLIST, "", "playlist", seq++)
        .label("&MusicBox")
        .description("Music player")
        .icon(theme->icon("musicbox", "group.music"))
        .install();

    seq = 0;
    action(ID_IMPORT_FILES, "playlist", "import_files", seq++)
        .label("&Import Files...")
        .description("Import audio files")
        .icon(theme->icon("musicbox", "file.import_files"))
        .performFn([this](PerformContext*) { importFiles(); })
        .install();

    action(ID_IMPORT_DIR, "playlist", "import_dir", seq++)
        .label("Import &Directory...")
        .description("Import all audio files in a directory")
        .icon(theme->icon("musicbox", "file.import_dir"))
        .performFn([this](PerformContext*) { importDirectory(); })
        .install();

    action(ID_CLEAR_PLAYLIST, "playlist", "clear", seq++)
        .label("&Clear Playlist")
        .description("Clear the current playlist")
        .icon(theme->icon("musicbox", "edit.clear"))
        .performFn([this](PerformContext*) { clearPlaylist(); })
        .install();

    state(ID_REPEAT, "playlist", "repeat", 10)
        .label("&Repeat")
        .description("Repeat current track when it reaches the end")
        .shortcut("Ctrl+R")
        .icon(theme->icon("musicbox", "media.repeat"))
        .stateType(UIStateType::BOOL)
        .valueDescriptorFn(UIState::ValueDescriptorFn(
            [this, theme](const UIStateVariant& nv) -> UIStateValueDescriptor {
                UIStateValueDescriptor d;
                d.label = m_repeat ? "Repeat" : "No Repeat";
                d.description = m_repeat ? "Repeat current track when it reaches the end"
                                         : "No repeat when playback reaches the end";
                d.icon = m_repeat ? theme->icon("musicbox", "media.repeat")
                                  : theme->icon("musicbox", "media.once");
                return d;
            }))
        .initValue(UIStateVariant{false})
        .valueRef(&m_repeatStateSink)
        .connect([this](const UIStateVariant& nv, const UIStateVariant&) {
            if (const bool* b = std::get_if<bool>(&nv))
                m_repeat = *b;
        })
        .install();

    state(ID_SHUFFLE, "playlist", "shuffle", 11)
        .label("Shuffle")
        .description("Shuffle playback order")
        .shortcut("Ctrl+S")
        .icon(theme->icon("musicbox", "media.shuffle"))
        .stateType(UIStateType::BOOL)
        .valueDescriptorFn(UIState::ValueDescriptorFn(
            [this, theme](const UIStateVariant& nv) -> UIStateValueDescriptor {
                UIStateValueDescriptor d;
                d.label = m_shuffle ? "Shuffle" : "Ordered";
                d.description = m_shuffle ? "Shuffle the playback order"
                                          : "Play the tracks in the order they are in the playlist";
                d.icon = m_shuffle ? theme->icon("musicbox", "media.shuffle")
                                   : theme->icon("musicbox", "media.ordered");
                return d;
            }))
        .initValue(UIStateVariant{false})
        .valueRef(&m_shuffleStateSink)
        .connect([this](const UIStateVariant& nv, const UIStateVariant&) {
            if (const bool* b = std::get_if<bool>(&nv))
                m_shuffle = *b;
        })
        .install();
    action(ID_REWIND, "media", "rewind", seq++)
        .label("&Previous")
        .description("Previous track")
        .icon(theme->icon("musicbox", "media.rewind"))
        .performFn([this](PerformContext*) { nextTrack(-1); })
        .install();

    action(ID_PLAY_PAUSE, "media", "play_pause", seq++)
        .label("&Play/Pause")
        .description("Toggle play/pause")
        .icon(theme->icon("musicbox", "media.play_pause"))
        .performFn([this](PerformContext*) {
            if (!m_media || m_queue.empty())
                return;
            const auto st = m_media->GetState();
            if (st == wxMEDIASTATE_PLAYING)
                m_media->Pause();
            else if (st == wxMEDIASTATE_PAUSED)
                m_media->Play();
            else
                openMediaForCurrent(true);
            updateTransportUi();
        })
        .install();
    action(ID_STOP, "media", "stop", seq++)
        .label("&Stop")
        .description("Stop playback")
        .icon(theme->icon("musicbox", "media.stop"))
        .performFn([this](PerformContext*) { stopPlayback(); })
        .install();
    action(ID_FORWARD, "media", "forward", seq++)
        .label("&Next")
        .description("Next track")
        .icon(theme->icon("musicbox", "media.forward"))
        .performFn([this](PerformContext*) { nextTrack(1); })
        .install();
}

MusicBoxBody::~MusicBoxBody() {
    if (m_progressTimer && m_progressTimer->IsRunning())
        m_progressTimer->Stop();
}

wxWindow* MusicBoxBody::createFragmentView(CreateViewContext* ctx) {
    m_frame = ctx->findParentFrame();
    wxWindow* parent = ctx->getParent();

    if (!m_vm) {
        if (auto* sh = ShellApp::getInstance())
            m_vm = sh->getVolumeManager();
    }
    if (!m_vm)
        return nullptr;

    m_root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    m_root->SetMinSize(wxSize(880, 560));

    // Library column.
    auto* left = new wxPanel(m_root, wxID_ANY);

    auto* libTop = new wxBoxSizer(wxHORIZONTAL);
    auto* chooseBtn = new wxButton(left, wxID_ANY, "Library Folder...");
    chooseBtn->Bind(wxEVT_BUTTON, &MusicBoxBody::onChooseLibraryFolder, this);
    libTop->Add(chooseBtn, 0, wxALL, 6);
    libTop->AddStretchSpacer();

    auto* outer = new wxBoxSizer(wxVERTICAL);
    auto* main = new wxBoxSizer(wxHORIZONTAL);
    auto* leftSizer = new wxBoxSizer(wxVERTICAL);
    leftSizer->Add(libTop, 0, wxEXPAND);
    leftSizer->Add(new wxStaticText(left, wxID_ANY, "No default volume"), 0, wxALL, 8);

    // Default library folder.
    Volume* anchor = m_vm->getDefaultVolume();
    if (!anchor) {
        m_root->SetSizer(outer);
        return nullptr;
    }

    m_libraryDir = VolumeFile(anchor, kDefaultLibraryPath);
    try {
        m_libraryDir->createDirectories();
    } catch (...) {
    }

    Location loc(m_libraryDir->getVolume(), m_libraryDir->getPath());
    m_libraryView = new FileListView(left, loc);
    m_libraryView->setViewMode("list");
    m_libraryView->setEntryFilter([this](const DirEntry& e) {
        if (!e.isRegularFile())
            return false;
        return isAudioExt(lowerExtOf(e.name));
    });
    m_libraryView->onFileActivated([this](VolumeFile& f) { onLibraryFileActivated(f); });
    m_libraryView->SetMinSize(wxSize(320, 360));

    main->Add(left, 0, wxEXPAND);

    // Playlist column.
    auto* right = new wxPanel(m_root, wxID_ANY);
    auto* rightSizer = new wxBoxSizer(wxVERTICAL);

    m_playlist = new wxListCtrl(right, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL);
    m_playlist->InsertColumn(0, "Track", wxLIST_FORMAT_LEFT, 360);
    m_playlist->InsertColumn(1, "Ext", wxLIST_FORMAT_LEFT, 80);

    auto* controls = new wxPanel(right, wxID_ANY);

    auto* progRow = new wxBoxSizer(wxHORIZONTAL);
    m_seekSlider = new wxSlider(controls, wxID_ANY, 0, 0, kGaugeRange, wxDefaultPosition,
                                wxSize(-1, 22), wxSL_HORIZONTAL);
    m_seekSlider->Bind(wxEVT_SLIDER, &MusicBoxBody::onSeek, this);
    m_seekSlider->Enable(false);

    m_timeLabel = new wxStaticText(controls, wxID_ANY, "00:00 / 00:00");
    progRow->Add(m_seekSlider, 1, wxEXPAND | wxLEFT | wxRIGHT, 6);
    progRow->Add(m_timeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_progressGauge =
        new wxGauge(controls, wxID_ANY, kGaugeRange, wxDefaultPosition, wxSize(-1, 10));
    m_progressGauge->SetValue(0);

    // Hidden media control used for playback.
    m_media = new wxMediaCtrl(right, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(1, 1), 0);
    m_media->Show(false);
    m_media->Bind(wxEVT_MEDIA_FINISHED, &MusicBoxBody::onMediaFinished, this);

    // Periodic UI refresh (progress).
    updateTransportUi();

    leftSizer->Add(m_libraryView, 1, wxEXPAND | wxALL, 6);
    left->SetSizer(leftSizer);
    rightSizer->Add(m_playlist, 1, wxEXPAND | wxALL, 6);
    auto* cSizer = new wxBoxSizer(wxVERTICAL);
    cSizer->Add(progRow, 0, wxEXPAND | wxTOP, 4);

    cSizer->Add(m_progressGauge, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    controls->SetSizer(cSizer);
    rightSizer->Add(controls, 0, wxEXPAND | wxALL, 6);

    m_progressTimer = new wxTimer(m_root, wxID_ANY);
    m_root->Bind(wxEVT_TIMER, &MusicBoxBody::onProgressTick, this, m_progressTimer->GetId());
    m_progressTimer->Start(kSeekTimerMs);

    right->SetSizer(rightSizer);
    main->Add(right, 1, wxEXPAND);
    outer->Add(main, 1, wxEXPAND);

    outer->SetSizeHints(parent);
    m_root->SetSizer(outer);

    loadPlaylist();
    m_frame->Bind(wxEVT_CLOSE_WINDOW, &MusicBoxBody::onFrameClose, this);

    refreshLibrary();

    return m_root;
}

void MusicBoxBody::refreshLibrary() {
    if (!m_libraryView || !m_libraryDir)
        return;
    Location loc(m_libraryDir->getVolume(), m_libraryDir->getPath());
    m_libraryView->setLocation(loc);
    m_libraryView->refresh();
}

void MusicBoxBody::setLibraryDir(const VolumeFile& dir) {
    if (!m_vm)
        return;
    m_libraryDir = dir;
    refreshLibrary();
}

void MusicBoxBody::onChooseLibraryFolder(wxCommandEvent&) {
    if (!m_vm || !m_frame)
        return;
    auto cur = m_libraryDir ? wxString(m_libraryDir->getPath()) : wxString("/");
    ChooseFolderDialog dlg(m_frame, m_vm, "MusicBox Settings", "Choose music folder", cur);
    dlg.setShowNewFolderButton(true);
    dlg.setMustExist(false);
    if (dlg.ShowModal() != wxID_OK)
        return;
    auto opt = dlg.getVolumeFile();
    if (!opt)
        return;

    setLibraryDir(*opt);
}

void MusicBoxBody::onLibraryFileActivated(VolumeFile& f) {
    // Double click: replace queue with single file and play.
    m_queue.clear();
    m_playlist->DeleteAllItems();
    addToQueue(f, true);
}

void MusicBoxBody::addToQueue(VolumeFile& f, bool autoplay) {
    if (f.getPath().empty())
        return;
    const std::string ext = lowerExtOf(f.getPath());
    if (!isAudioExt(ext))
        return;

    // De-dupe by path.
    for (const auto& q : m_queue) {
        if (q.getPath() == f.getPath() && q.getVolume() == f.getVolume())
            return;
    }

    m_queue.push_back(f);
    const long row = m_playlist->GetItemCount();
    const std::string path = f.getPath();
    const size_t slash = path.find_last_of('/');
    const std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);

    m_playlist->InsertItem(row, wxString::FromUTF8(base.c_str()));
    m_playlist->SetItem(row, 1, wxString::FromUTF8(ext.c_str()));

    if (!m_hasCurrent) {
        m_hasCurrent = true;
        m_currentIndex = 0;
        m_playlist->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }

    if (autoplay)
        playIndex(m_currentIndex);

    savePlaylist();
}

void MusicBoxBody::playIndex(size_t idx) {
    if (m_queue.empty() || idx >= m_queue.size())
        return;
    m_currentIndex = idx;
    m_hasCurrent = true;
    openMediaForCurrent(true);
    updateTransportUi();
    // Select row.
    for (long i = 0; i < m_playlist->GetItemCount(); ++i)
        m_playlist->SetItemState(i, 0, wxLIST_STATE_SELECTED);
    m_playlist->SetItemState(static_cast<long>(idx), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    m_playlist->EnsureVisible(static_cast<long>(idx));
    savePlaylist();
}

void MusicBoxBody::openMediaForCurrent(bool autoplay) {
    if (!m_media || m_queue.empty())
        return;
    ShellApp* sh = ShellApp::getInstance();
    if (!sh || !sh->vfsDaemon().isRunning())
        return;
    const VolumeFile& f = m_queue[m_currentIndex];
    const int idx = volumeIndexFor(m_vm, f.getVolume());
    if (idx < 0)
        return;
    std::string rel = stripLeadingSlash(f.getPath());
    const std::string url = sh->vfsDaemon().httpUrlForVolumePath(static_cast<size_t>(idx), rel);
    if (url.empty())
        return;
    wxURI u(wxString::FromUTF8(url.data(), static_cast<int>(url.size())));
    if (!m_media->Load(u))
        return;
    if (autoplay)
        m_media->Play();

    updateProgressUi();
}

void MusicBoxBody::stopPlayback() {
    if (!m_media)
        return;
    m_media->Stop();
    updateTransportUi();
    updateProgressUi();
}

void MusicBoxBody::nextTrack(int delta) {
    if (m_queue.empty())
        return;
    if (delta == 0)
        return;

    size_t nextIdx = m_currentIndex;
    if (m_shuffle) {
        nextIdx = pickShuffleIndex(m_currentIndex);
    } else {
        const int n = static_cast<int>(m_queue.size());
        const int cur = static_cast<int>(m_currentIndex);
        nextIdx = static_cast<size_t>((cur + delta + n) % n);
    }
    playIndex(nextIdx);
}

size_t MusicBoxBody::pickShuffleIndex(size_t current) {
    if (m_queue.size() < 2)
        return current;
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<size_t> dist(0, m_queue.size() - 1);
    for (int i = 0; i < 8; ++i) {
        size_t idx = dist(rng);
        if (idx != current)
            return idx;
    }
    return (current + 1) % m_queue.size();
}

void MusicBoxBody::updateTransportUi() {
    // Actions are exposed via UIAction system; UI widget state is handled by progress elements.
}

void MusicBoxBody::updateProgressUi() {
    if (!m_media)
        return;
    const auto tell = m_media->Tell();
    const auto len = m_media->Length();

    const long long tellMs = static_cast<long long>(tell);
    const long long lenMs = static_cast<long long>(len);
    if (lenMs <= 0) {
        if (m_timeLabel)
            m_timeLabel->SetLabel("00:00 / 00:00");
        if (m_seekSlider)
            m_seekSlider->Enable(false);
        if (m_progressGauge) {
            m_progressGauge->SetValue(0);
            m_progressGauge->Enable(false);
        }
        return;
    }

    const int tSec = static_cast<int>(tellMs / 1000);
    const int lSec = static_cast<int>(lenMs / 1000);

    auto fmt = [](int sec) {
        int m = sec / 60;
        int s = sec % 60;
        return wxString::Format("%02d:%02d", m, s);
    };

    if (m_timeLabel)
        m_timeLabel->SetLabel(fmt(tSec) + " / " + fmt(lSec));

    if (m_seekSlider) {
        m_seekSlider->Enable(true);
        const int range = kGaugeRange;
        int slider =
            static_cast<int>((static_cast<double>(tellMs) / static_cast<double>(lenMs)) * range);
        slider = std::clamp(slider, 0, range);
        // Prevent slider updates from triggering onSeek() while we refresh progress.
        const bool wasSeeking = m_userSeeking;
        m_userSeeking = true;
        m_seekSlider->SetValue(slider);
        m_userSeeking = wasSeeking;
    }

    if (m_progressGauge) {
        m_progressGauge->Enable(true);
        const int v = m_seekSlider ? m_seekSlider->GetValue() : 0;
        m_progressGauge->SetValue(std::clamp(v, 0, kGaugeRange));
    }
}

void MusicBoxBody::onMediaFinished(wxMediaEvent&) {
    if (m_repeat && m_queue.size() > 0) {
        if (m_media) {
            m_media->Seek(0);
            m_media->Play();
        }
        return;
    }
    nextTrack(1);
}

void MusicBoxBody::onProgressTick(wxTimerEvent&) {
    if (m_userSeeking)
        return;
    updateProgressUi();
}

void MusicBoxBody::onSeek(wxCommandEvent&) {
    if (!m_media || !m_seekSlider)
        return;
    if (!m_libraryDir)
        return;

    // Ignore seeks triggered by programmatic slider updates.
    if (m_userSeeking)
        return;

    m_userSeeking = true;
    const auto len = m_media->Length();
    const long long lenMs = static_cast<long long>(len);
    if (lenMs > 0) {
        const int range = kGaugeRange;
        const int v = m_seekSlider->GetValue();
        const long long targetMs = (static_cast<long long>(v) * lenMs) / range;
        m_media->Seek(static_cast<wxFileOffset>(targetMs), wxFromStart);
    }

    updateProgressUi();
    m_userSeeking = false;
}

void MusicBoxBody::onFrameClose(wxCloseEvent& e) {
    savePlaylist();
    e.Skip();
}

void MusicBoxBody::clearPlaylist() {
    stopPlayback();
    m_queue.clear();
    m_hasCurrent = false;
    m_currentIndex = 0;
    if (m_playlist)
        m_playlist->DeleteAllItems();
    savePlaylist();
    updateProgressUi();
}

Volume* MusicBoxBody::findVolumeById(const std::string& id) const {
    if (!m_vm)
        return nullptr;
    for (size_t i = 0; i < m_vm->getVolumeCount(); ++i) {
        Volume* v = m_vm->getVolume(i);
        if (v && v->getUUID() == id)
            return v;
    }
    return nullptr;
}

void MusicBoxBody::loadPlaylist() {
    if (!m_vm || !m_playlist)
        return;

    IRegistry& r = registry();
    const std::string raw = r.getString(kPlaylistKey, "");
    if (raw.empty())
        return;

    clearPlaylist();

    std::istringstream iss(raw);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;
        const size_t sep = line.find('|');
        if (sep == std::string::npos)
            continue;
        const std::string vid = line.substr(0, sep);
        const std::string path = line.substr(sep + 1);
        Volume* v = findVolumeById(vid);
        if (!v)
            continue;
        VolumeFile vf(v, path);
        addToQueue(vf, false);
    }

    const long idx = r.getLong(kPlaylistIndexKey, 0);
    if (!m_queue.empty()) {
        const size_t i =
            static_cast<size_t>(std::clamp<long>(idx, 0, static_cast<long>(m_queue.size() - 1)));
        m_currentIndex = i;
        m_hasCurrent = true;
        m_playlist->SetItemState(static_cast<long>(i), wxLIST_STATE_SELECTED,
                                 wxLIST_STATE_SELECTED);
        m_playlist->EnsureVisible(static_cast<long>(i));
    }
}

void MusicBoxBody::savePlaylist() {
    if (!m_vm)
        return;
    IRegistry& r = registry();

    std::ostringstream oss;
    for (size_t i = 0; i < m_queue.size(); ++i) {
        Volume* v = m_queue[i].getVolume();
        if (!v)
            continue;
        if (oss.tellp() > 0)
            oss << "\n";
        oss << v->getUUID() << "|" << m_queue[i].getPath();
    }
    r.set(kPlaylistKey, oss.str());
    r.set(kPlaylistIndexKey, static_cast<long>(m_hasCurrent ? m_currentIndex : 0));
    r.save();
}

void MusicBoxBody::importFiles() {
    if (!m_vm || !m_frame)
        return;
    ChooseFileDialog dlg(m_frame, m_vm, "Import Audio Files", FileDialogMode::Open,
                         m_libraryDir ? wxString(m_libraryDir->getPath()) : wxString("/"), "");
    dlg.addFilter("Audio", "*.mp3");
    dlg.addFilter("Audio", "*.wav");
    dlg.addFilter("Audio", "*.flac");
    dlg.addFilter("Audio", "*.ogg");
    dlg.setMultiSelect(true);
    dlg.setFileMustExist(true);
    if (dlg.ShowModal() != wxID_OK)
        return;
    auto files = dlg.getVolumeFiles();
    for (auto& vf : files)
        addToQueue(vf, false);
    if (!files.empty() && !m_hasCurrent) {
        m_hasCurrent = true;
        m_currentIndex = 0;
    }
    savePlaylist();
}

void MusicBoxBody::importDirectory() {
    if (!m_vm || !m_frame)
        return;
    ChooseFolderDialog dlg(m_frame, m_vm, "Import Directory", "Choose directory to scan",
                           m_libraryDir ? wxString(m_libraryDir->getPath()) : wxString("/"));
    dlg.setShowNewFolderButton(false);
    dlg.setMustExist(true);
    if (dlg.ShowModal() != wxID_OK)
        return;
    auto opt = dlg.getVolumeFile();
    if (!opt)
        return;
    Volume* vol = opt->getVolume();
    if (!vol)
        return;

    std::vector<std::string> paths;
    importDirectoryRecursive(vol, opt->getPath(), 0, paths);
    std::sort(paths.begin(), paths.end());
    for (const auto& p : paths) {
        VolumeFile vf(vol, p);
        addToQueue(vf, false);
    }
    savePlaylist();
}

void MusicBoxBody::importDirectoryRecursive(Volume* vol, const std::string& dirPath, int depth,
                                            std::vector<std::string>& outFiles) {
    if (!vol)
        return;
    if (depth > 64)
        return;
    try {
        if (!vol->isDirectory(dirPath))
            return;
        auto entries = vol->readDir(dirPath);
        for (const auto& [name, st] : entries->children) {
            if (!st)
                continue;
            if (st->name == "." || st->name == "..")
                continue;
            std::string p = dirPath;
            if (p.empty() || p.back() != '/')
                p += "/";
            p += st->name;
            if (st->isDirectory()) {
                importDirectoryRecursive(vol, p, depth + 1, outFiles);
            } else if (st->isRegularFile()) {
                if (isAudioExt(lowerExtOf(st->name)))
                    outFiles.push_back(p);
            }
        }
    } catch (...) {
    }
}

} // namespace os

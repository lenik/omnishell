#ifndef OMNISHELL_MOD_MUSICBOX_MUSICBOXBODY_HPP
#define OMNISHELL_MOD_MUSICBOX_MUSICBOXBODY_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/ui/arch/UIState.hpp>
#include <bas/wx/uiframe.hpp>

#include <bas/volume/VolumeFile.hpp>

#include <optional>
#include <string>
#include <vector>

class wxMediaCtrl;
class wxMediaEvent;
class wxFileDialog;
class wxListCtrl;
class wxSlider;
class wxTimer;
class wxStaticText;
class wxPanel;
class wxGauge;

class FileListView;
class VolumeManager;
namespace os {

enum class MusicBoxPlayMode { Ordered, Shuffle };

class MusicBoxBody : public UIFragment {
public:
    explicit MusicBoxBody(VolumeManager* vm);
    ~MusicBoxBody() override;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

private:
    void refreshLibrary();
    void setLibraryDir(const VolumeFile& dir);

    void loadPlaylist();
    void savePlaylist();
    void clearPlaylist();
    Volume* findVolumeById(const std::string& id) const;

    void importFiles();
    void importDirectory();
    void importDirectoryRecursive(Volume* vol, const std::string& dirPath, int depth,
                                 std::vector<std::string>& outFiles);

    void addToQueue(VolumeFile& f, bool autoplay);
    void playIndex(size_t idx);
    void stopPlayback();
    void nextTrack(int delta);
    size_t pickShuffleIndex(size_t current);

    void openMediaForCurrent(bool autoplay);
    void updateTransportUi();
    void updateProgressUi();

    void onLibraryFileActivated(VolumeFile& f);
    void onChooseLibraryFolder(wxCommandEvent& e);

    void onMediaFinished(wxMediaEvent& e);
    void onProgressTick(wxTimerEvent& e);
    void onSeek(wxCommandEvent& e);
    void onFrameClose(wxCloseEvent& e);

private:
    uiFrame* m_frame{nullptr};
    wxPanel* m_root{nullptr};
    VolumeManager* m_vm{nullptr};

    // Library folder and file list.
    std::optional<VolumeFile> m_libraryDir;
    FileListView* m_libraryView{nullptr};

    // Playback queue and UI list.
    std::vector<VolumeFile> m_queue;
    size_t m_currentIndex{0};
    bool m_hasCurrent{false};

    wxListCtrl* m_playlist{nullptr};

    // Media and UI controls.
    wxMediaCtrl* m_media{nullptr};
    wxTimer* m_progressTimer{nullptr};
    wxSlider* m_seekSlider{nullptr};
    wxStaticText* m_timeLabel{nullptr};
    wxGauge* m_progressGauge{nullptr};

    bool m_userSeeking{false};
    bool m_repeat{false};
    bool m_shuffle{false};

    /** Required by UIState::build(); see bas UIState::_Builder::m_valueRef. */
    observable<UIStateVariant>* m_repeatStateSink = nullptr;
    observable<UIStateVariant>* m_shuffleStateSink = nullptr;

    // Base folder for library selection.
    static constexpr const char* kDefaultLibraryPath = "/Music";
};

} // namespace os

#endif // OMNISHELL_MOD_MUSICBOX_MUSICBOXBODY_HPP


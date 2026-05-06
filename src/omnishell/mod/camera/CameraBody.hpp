#ifndef OMNISHELL_MOD_CAMERA_CAMERABODY_HPP
#define OMNISHELL_MOD_CAMERA_CAMERABODY_HPP

#include "CameraGstPreview.hpp"

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>
#include <bas/wx/uiframe.hpp>

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

class wxButton;
class wxKeyEvent;
class wxMediaCtrl;
class wxStaticBitmap;
class wxStaticText;
class wxRadioBox;
class wxPanel;
class wxTimer;

class FileListView;

namespace os {

enum class CameraMode { Photo, Video };

class CameraBody : public UIFragment {
public:
    CameraBody();
    ~CameraBody() override;

    void createFragmentView(CreateViewContext* ctx) override;

private:
    void onModeChanged(wxCommandEvent& e);
    void refreshModeUi();

    void capturePhoto();
    void recordVideo();
    void pauseOrReplayVideo(); // SPACE: pause/resume, or replay if stopped
    void stopVideo();          // ENTER: stop (or no-op if already stopped)
    void replayVideo();        // replay from the start

    void onOpenGalleryItem(VolumeFile& file);
    void onSettings();
    void onPreviewTick(wxTimerEvent& e);
    void OnKeyDown(wxKeyEvent& event);

    void ensureDestinationDir();
    void scanRecent();
    void applyGalleryFilterAndRefresh();
    bool extMatches(const std::string& ext) const;

    int nextDcfCounterForPrefix(const std::string& prefix) const;
    std::string dcfFilename(const std::string& prefix, int counter, const std::string& extUpper) const;
    std::string dcfPhotoExtUpper(const std::string& srcExtLower) const;
    std::string dcfVideoExtUpper(const std::string& srcExtLower) const;

    wxString nowStamp() const;
    wxSize fitToBoxMaintainAspect(int srcW, int srcH, const wxSize& box) const;
    std::string lowerExtOf(const std::string& path) const;

    void togglePreviewPlaying();
    void deleteLastCapture();
    void navigateGallery(int delta);

private:
    uiFrame* m_frame{nullptr};
    wxPanel* m_root{nullptr};

    CameraMode m_mode{CameraMode::Photo};
    static constexpr size_t kMaxRecent = 14;
    std::unordered_set<std::string> m_recentPhotoNames;
    std::unordered_set<std::string> m_recentVideoNames;
    std::vector<std::string> m_recentPhotoOrder;
    std::vector<std::string> m_recentVideoOrder;

    // Destination dir where captures are saved.
    VolumeManager* m_vm{nullptr};
    std::optional<VolumeFile> m_destDir;

    wxRadioBox* m_modeBox{nullptr};
    wxStaticBitmap* m_photoPreview{nullptr};

#if HAVE_WX_MEDIA
    wxMediaCtrl* m_media{nullptr};
#endif
    bool m_videoLoaded{false};
    wxStaticText* m_videoStatus{nullptr};

    FileListView* m_gallery{nullptr};

    std::unique_ptr<CameraGstPreview> m_gstPreview;
    wxTimer* m_previewTimer{nullptr};
    /** Cleared first in ~CameraBody so a queued timer cannot touch GStreamer/UI after teardown. */
    bool m_previewActive{true};
    /** SPACE/F2 toggle: if false, freeze preview updates (last frame stays on screen). */
    bool m_previewPlaying{true};

    /** Most recently created capture/record file (for BACK delete shortcut). */
    std::optional<VolumeFile> m_lastSavedFile;

    // Latest preview frame cache for instant capture without dialog fallback.
    std::vector<uint8_t> m_lastPreviewRgb;
    int m_lastPreviewW{0};
    int m_lastPreviewH{0};
};

} // namespace os

#endif // OMNISHELL_MOD_CAMERA_CAMERABODY_HPP


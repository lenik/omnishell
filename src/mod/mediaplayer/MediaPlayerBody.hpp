#ifndef OMNISHELL_MOD_MEDIAPLAYER_BODY_HPP
#define OMNISHELL_MOD_MEDIAPLAYER_BODY_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/ui/arch/UIState.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/wx/uiframe.hpp>

#include <optional>

#include <wx/event.h>
#include <wx/longlong.h>
#include <wx/timer.h>

class VolumeManager;

#if HAVE_WX_MEDIA
#include <wx/mediactrl.h>
#else
class wxMediaCtrl;
#endif

namespace os {

class MediaPlayerBody : public UIFragment {
  public:
    explicit MediaPlayerBody(VolumeManager* vm);
    ~MediaPlayerBody() override;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

    /** Load file from explorer / app open (autoplay). */
    void openVolumePath(size_t volumeIndex, const std::string& pathOnVolume);

    /** Current playback position in ms (0 if unknown). */
    wxLongLong playbackTellMs() const;

    bool isPlaybackPlaying() const;

  private:
    void onOpen(PerformContext* ctx);
    void onPlay(PerformContext* ctx);
    void onPause(PerformContext* ctx);
    void onStop(PerformContext* ctx);

    void onFrameClose(wxCloseEvent& event);
    void onVizTimer(wxTimerEvent& event);
#if HAVE_WX_MEDIA
    void onMediaFinished(wxMediaEvent& event);
#endif

    void loadVolumeFile(const VolumeFile& file, bool autoplay);
    void updateFrameTitle();
    void syncVisualizerLayout();
    void refreshVisualizer();

    VolumeManager* m_vm;
    uiFrame* m_frame = nullptr;
    wxPanel* m_root = nullptr;
    wxPanel* m_visualizer = nullptr;
#if HAVE_WX_MEDIA
    wxMediaCtrl* m_media = nullptr;
#endif
    wxTimer m_vizTimer;

    std::optional<VolumeFile> m_file;
    bool m_repeat = false;
    bool m_audioSpectrum = false;

    /** Required by UIState::build(); see bas UIState::_Builder::m_valueRef (uninitialized otherwise). */
    observable<UIStateVariant>* m_repeatStateSink = nullptr;
    observable<UIStateVariant>* m_spectrumStateSink = nullptr;
};

} // namespace os

#endif

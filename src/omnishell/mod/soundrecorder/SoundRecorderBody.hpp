#ifndef OMNISHELL_MOD_SOUNDRECORDER_BODY_HPP
#define OMNISHELL_MOD_SOUNDRECORDER_BODY_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/timer.h>

#include <cstdint>
#include <vector>

#if HAVE_WX_MEDIA
#include <wx/mediactrl.h>
#endif

class wxButton;
class wxPanel;
class wxStaticText;

namespace os {

/** Idle: no capture session. Recording/Paused: session active. Stopped: capture finished, buffer kept. */
enum class SoundRecState { Idle, Recording, Paused, Stopped };

class SoundRecorderBody : public UIFragment {
public:
    SoundRecorderBody() = default;
    ~SoundRecorderBody() override;

    void createFragmentView(CreateViewContext* ctx) override;

private:
    void OnStart(wxCommandEvent& event);
    void OnPauseResume(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnReplay(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnTick(wxTimerEvent& event);
#if HAVE_WX_MEDIA
    void OnMediaFinished(wxMediaEvent& event);
#endif

    void updateButtonStates();
    void updateTimerLabel();
    void tickCapture();
    void ensureMicOpen();
    void closeMic();

    uiFrame* m_frame{nullptr};
    wxPanel* m_panel{nullptr};
    wxPanel* m_waveform{nullptr};
    wxStaticText* m_timerDisplay{nullptr};
    wxButton* m_startBtn{nullptr};
    wxButton* m_pauseBtn{nullptr};
    wxButton* m_stopBtn{nullptr};
    wxButton* m_replayBtn{nullptr};
    wxButton* m_saveBtn{nullptr};
#if HAVE_WX_MEDIA
    wxMediaCtrl* m_media{nullptr};
#endif

    wxTimer m_tickTimer;
    SoundRecState m_state{SoundRecState::Idle};
    std::vector<int16_t> m_pcmData;
    int m_sampleRate{44100};
    double m_recordSeconds{0};
    double m_synthPhase{0};
#if defined(OMNISHELL_HAVE_ALSA)
    void* m_alsaPcm{nullptr};
#endif
};

} // namespace os

#endif

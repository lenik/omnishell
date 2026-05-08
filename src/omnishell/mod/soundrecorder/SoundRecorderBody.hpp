#ifndef OMNISHELL_MOD_SOUNDRECORDER_BODY_HPP
#define OMNISHELL_MOD_SOUNDRECORDER_BODY_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/mediactrl.h>
#include <wx/timer.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/stattext.h>

#include <cstdint>
#include <vector>

namespace os {

/** Idle: no capture session. Recording/Paused: session active. Stopped: capture finished, buffer kept. */
enum class SoundRecState { Idle, Recording, Paused, Stopped };

class SoundRecorderBody : public UIFragment {
public:
    SoundRecorderBody() = default;
    ~SoundRecorderBody() override;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

private:
    void OnStart(wxCommandEvent& event);
    void OnPauseResume(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnReplay(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnTick(wxTimerEvent& event);
    void OnMediaFinished(wxMediaEvent& event);

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
    wxMediaCtrl* m_media{nullptr};

    wxTimer m_tickTimer;
    SoundRecState m_state{SoundRecState::Idle};
    std::vector<int16_t> m_pcmData;
    int m_sampleRate{44100};
    double m_recordSeconds{0};
    double m_synthPhase{0};
    void* m_alsaPcm{nullptr};
};

} // namespace os

#endif

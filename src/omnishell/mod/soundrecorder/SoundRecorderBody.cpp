#include "SoundRecorderBody.hpp"

#include <wx/button.h>
#include <wx/dcbuffer.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#if defined(OMNISHELL_HAVE_ALSA)
#include <alsa/asoundlib.h>
#endif

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <deque>
#include <vector>

namespace os {

namespace {

enum {
    ID_START = wxID_HIGHEST + 1,
    ID_PAUSE,
    ID_STOP,
    ID_REPLAY,
    ID_SAVE
};

constexpr int kTickMs = 50;
constexpr int kWaveMaxPoints = 4000;

class WaveformPanel : public wxPanel {
  public:
    explicit WaveformPanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize(-1, 120));
        Bind(wxEVT_PAINT, &WaveformPanel::OnPaint, this);
    }

    void PushAmplitude(float v) {
        v = std::clamp(v, 0.f, 1.f);
        m_peaks.push_back(v);
        while (m_peaks.size() > kWaveMaxPoints)
            m_peaks.pop_front();
        Refresh(false);
    }

    void ClearWaveform() {
        m_peaks.clear();
        Refresh(false);
    }

  private:
    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        const wxSize sz = GetClientSize();
        dc.SetBrush(wxColour(24, 26, 32));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, sz.x, sz.y);
        if (m_peaks.size() < 2)
            return;
        dc.SetPen(wxPen(wxColour(72, 210, 130), 1));
        const int mid = sz.y / 2;
        const int amp = std::max(1, mid - 2);
        const float nx = static_cast<float>(sz.x);
        const size_t n = m_peaks.size();
        for (size_t i = 1; i < n; ++i) {
            const float x0 = static_cast<float>(i - 1) / static_cast<float>(n - 1) * nx;
            const float x1 = static_cast<float>(i) / static_cast<float>(n - 1) * nx;
            const float y0 = static_cast<float>(mid) - m_peaks[i - 1] * static_cast<float>(amp);
            const float y1 = static_cast<float>(mid) - m_peaks[i] * static_cast<float>(amp);
            dc.DrawLine(wxRound(x0), wxRound(y0), wxRound(x1), wxRound(y1));
        }
    }

    std::deque<float> m_peaks;
};

static void pushPeaksFromSamples(WaveformPanel* wp, const int16_t* s, int n) {
    if (!wp || n <= 0)
        return;
    const int stride = std::max(32, n / 24);
    for (int i = 0; i < n; i += stride) {
        const int end = std::min(i + stride, n);
        int maxv = 0;
        for (int j = i; j < end; ++j) {
            const int a = std::abs(static_cast<int>(s[j]));
            if (a > maxv)
                maxv = a;
        }
        wp->PushAmplitude(static_cast<float>(maxv) / 32768.f);
    }
}

static void pushSimulatedMic(WaveformPanel* wp, double& phase) {
    if (!wp)
        return;
    phase += 0.35;
    const float v =
        static_cast<float>((std::sin(phase) * 0.35 + 0.45) * (0.85 + 0.15 * std::sin(phase * 3.1)));
    wp->PushAmplitude(std::clamp(v, 0.f, 1.f));
}

static void writeLE16(wxFile& f, uint16_t v) {
    unsigned char b[2];
    b[0] = static_cast<unsigned char>(v & 0xff);
    b[1] = static_cast<unsigned char>((v >> 8) & 0xff);
    f.Write(b, 2);
}

static void writeLE32(wxFile& f, uint32_t v) {
    unsigned char b[4];
    b[0] = static_cast<unsigned char>(v & 0xff);
    b[1] = static_cast<unsigned char>((v >> 8) & 0xff);
    b[2] = static_cast<unsigned char>((v >> 16) & 0xff);
    b[3] = static_cast<unsigned char>((v >> 24) & 0xff);
    f.Write(b, 4);
}

static bool writePcmWav(const wxString& path, const std::vector<int16_t>& samples, int sampleRate) {
    if (samples.empty())
        return false;
    wxFile f(path, wxFile::write);
    if (!f.IsOpened())
        return false;

    const uint32_t numSamples = static_cast<uint32_t>(samples.size());
    const uint32_t dataSize = numSamples * sizeof(int16_t);
    const uint32_t chunkSize = 36 + dataSize;
    const uint16_t audioFormat = 1;
    const uint16_t numChannels = 1;
    const uint16_t bitsPerSample = 16;
    const uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    const uint32_t byteRate = static_cast<uint32_t>(sampleRate) * blockAlign;
    const uint16_t fmtChunkSize = 16;

    if (f.Write("RIFF", 4) != 4)
        return false;
    writeLE32(f, chunkSize);
    if (f.Write("WAVE", 4) != 4)
        return false;
    if (f.Write("fmt ", 4) != 4)
        return false;
    writeLE32(f, fmtChunkSize);
    writeLE16(f, audioFormat);
    writeLE16(f, numChannels);
    writeLE32(f, static_cast<uint32_t>(sampleRate));
    writeLE32(f, byteRate);
    writeLE16(f, blockAlign);
    writeLE16(f, bitsPerSample);
    if (f.Write("data", 4) != 4)
        return false;
    writeLE32(f, dataSize);
    return f.Write(samples.data(), dataSize) == dataSize;
}

} // namespace

SoundRecorderBody::~SoundRecorderBody() {
    if (m_tickTimer.IsRunning())
        m_tickTimer.Stop();
    if (m_frame)
        m_frame->Unbind(wxEVT_TIMER, &SoundRecorderBody::OnTick, this, m_tickTimer.GetId());
#if HAVE_WX_MEDIA
    if (m_media)
        m_media->Unbind(wxEVT_MEDIA_FINISHED, &SoundRecorderBody::OnMediaFinished, this);
#endif
    closeMic();
}

void SoundRecorderBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    m_panel = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    m_panel->SetMinSize(wxSize(420, 320));

    auto* mainsizer = new wxBoxSizer(wxVERTICAL);

    auto* wf = new WaveformPanel(m_panel);
    m_waveform = wf;
    mainsizer->Add(wf, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    m_timerDisplay =
        new wxStaticText(m_panel, wxID_ANY, "00:00.0", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    m_timerDisplay->SetFont(wxFont(28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    mainsizer->Add(m_timerDisplay, 0, wxALL | wxALIGN_CENTER, 8);

    auto* btnsizer = new wxBoxSizer(wxHORIZONTAL);
    m_startBtn = new wxButton(m_panel, ID_START, "Start");
    m_pauseBtn = new wxButton(m_panel, ID_PAUSE, "Pause");
    m_stopBtn = new wxButton(m_panel, ID_STOP, "Stop");
    m_replayBtn = new wxButton(m_panel, ID_REPLAY, "Replay");
    m_saveBtn = new wxButton(m_panel, ID_SAVE, "Save");

    btnsizer->Add(m_startBtn, 1, wxALL, 6);
    btnsizer->Add(m_pauseBtn, 1, wxALL, 6);
    btnsizer->Add(m_stopBtn, 1, wxALL, 6);
    btnsizer->Add(m_replayBtn, 1, wxALL, 6);
    btnsizer->Add(m_saveBtn, 1, wxALL, 6);

    mainsizer->Add(btnsizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);

#if HAVE_WX_MEDIA
    m_media = new wxMediaCtrl(m_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(1, 1), 0);
    m_media->Show(false);
    mainsizer->Add(m_media, 0, wxALL, 0);
    m_media->Bind(wxEVT_MEDIA_FINISHED, &SoundRecorderBody::OnMediaFinished, this);
#endif

    m_panel->SetSizer(mainsizer);

    m_panel->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        switch (e.GetId()) {
        case ID_START:
            OnStart(e);
            break;
        case ID_PAUSE:
            OnPauseResume(e);
            break;
        case ID_STOP:
            OnStop(e);
            break;
        case ID_REPLAY:
            OnReplay(e);
            break;
        case ID_SAVE:
            OnSave(e);
            break;
        default:
            break;
        }
    });

    m_tickTimer.SetOwner(m_frame);
    m_frame->Bind(wxEVT_TIMER, &SoundRecorderBody::OnTick, this, m_tickTimer.GetId());
    m_tickTimer.Start(kTickMs);

    ensureMicOpen();
    updateButtonStates();
    updateTimerLabel();
}

void SoundRecorderBody::ensureMicOpen() {
#if defined(OMNISHELL_HAVE_ALSA)
    if (m_alsaPcm)
        return;
    snd_pcm_t* pcm = nullptr;
    if (snd_pcm_open(&pcm, "default", SND_PCM_STREAM_CAPTURE, 0) < 0)
        return;
    if (snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1, m_sampleRate, 1,
                           500000)
        < 0) {
        snd_pcm_close(pcm);
        return;
    }
    snd_pcm_nonblock(pcm, 1);
    m_alsaPcm = pcm;
#endif
}

void SoundRecorderBody::closeMic() {
#if defined(OMNISHELL_HAVE_ALSA)
    if (m_alsaPcm) {
        snd_pcm_close(static_cast<snd_pcm_t*>(m_alsaPcm));
        m_alsaPcm = nullptr;
    }
#endif
}

void SoundRecorderBody::updateTimerLabel() {
    if (!m_timerDisplay)
        return;
    const int tenths = static_cast<int>(m_recordSeconds * 10.0 + 0.5) % 60000;
    const int s = tenths / 10;
    const int frac = tenths % 10;
    const int m = s / 60;
    const int rs = s % 60;
    m_timerDisplay->SetLabel(wxString::Format("%02d:%02d.%d", m, rs, frac));
}

void SoundRecorderBody::updateButtonStates() {
    const bool hasData = !m_pcmData.empty();
    const bool rec = m_state == SoundRecState::Recording;
    const bool paused = m_state == SoundRecState::Paused;
    const bool idle = m_state == SoundRecState::Idle;
    const bool stopped = m_state == SoundRecState::Stopped;

    m_startBtn->Enable(idle || stopped);
    m_pauseBtn->Enable(rec || paused);
    m_pauseBtn->SetLabel(paused ? "Resume" : "Pause");
    m_stopBtn->Enable(rec || paused);
    m_replayBtn->Enable(stopped && hasData);
    m_saveBtn->Enable(stopped && hasData);
}

void SoundRecorderBody::OnStart(wxCommandEvent&) {
    m_pcmData.clear();
    m_recordSeconds = 0;
    m_state = SoundRecState::Recording;
    if (auto* wf = dynamic_cast<WaveformPanel*>(m_waveform))
        wf->ClearWaveform();
    ensureMicOpen();
    updateButtonStates();
    updateTimerLabel();
}

void SoundRecorderBody::OnPauseResume(wxCommandEvent&) {
    if (m_state == SoundRecState::Recording)
        m_state = SoundRecState::Paused;
    else if (m_state == SoundRecState::Paused)
        m_state = SoundRecState::Recording;
    updateButtonStates();
}

void SoundRecorderBody::OnStop(wxCommandEvent&) {
    if (m_state != SoundRecState::Recording && m_state != SoundRecState::Paused)
        return;
    m_state = SoundRecState::Stopped;
    updateButtonStates();
}

void SoundRecorderBody::OnReplay(wxCommandEvent&) {
    if (m_pcmData.empty())
        return;
#if HAVE_WX_MEDIA
    wxString path = wxFileName::CreateTempFileName("omni_soundrec_");
    path += ".wav";
    if (!writePcmWav(path, m_pcmData, m_sampleRate)) {
        wxMessageBox("Could not prepare replay file.", "Sound Recorder", wxOK | wxICON_ERROR);
        return;
    }
    if (m_media && m_media->Load(path)) {
        m_media->Play();
    } else {
        wxMessageBox("Could not load recording for replay.", "Sound Recorder", wxOK | wxICON_ERROR);
    }
#else
    wxMessageBox("Replay requires wxWidgets built with media support (HAVE_WX_MEDIA).", "Sound Recorder",
                 wxOK | wxICON_INFORMATION);
#endif
}

void SoundRecorderBody::OnSave(wxCommandEvent&) {
    if (m_pcmData.empty())
        return;
    wxWindow* dlgParent = m_frame ? static_cast<wxWindow*>(m_frame) : m_panel;
    wxFileDialog dlg(dlgParent, "Save recording", "", "recording.wav", "WAV files (*.wav)|*.wav",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK)
        return;
    if (!writePcmWav(dlg.GetPath(), m_pcmData, m_sampleRate)) {
        wxMessageBox("Could not save file.", "Sound Recorder", wxOK | wxICON_ERROR);
        return;
    }
}

void SoundRecorderBody::OnTick(wxTimerEvent&) {
    tickCapture();
}

#if HAVE_WX_MEDIA
void SoundRecorderBody::OnMediaFinished(wxMediaEvent&) {
    if (m_media)
        m_media->Stop();
}
#endif

void SoundRecorderBody::tickCapture() {
    auto* wf = dynamic_cast<WaveformPanel*>(m_waveform);
    if (!wf)
        return;

    std::vector<int16_t> buf(4096);
    int got = 0;

#if defined(OMNISHELL_HAVE_ALSA)
    if (m_alsaPcm) {
        const snd_pcm_sframes_t n =
            snd_pcm_readi(static_cast<snd_pcm_t*>(m_alsaPcm), buf.data(), buf.size());
        if (n < 0) {
            if (n == -EPIPE)
                snd_pcm_prepare(static_cast<snd_pcm_t*>(m_alsaPcm));
            got = 0;
        } else {
            got = static_cast<int>(n);
        }
    }
#endif

    if (got > 0) {
        pushPeaksFromSamples(wf, buf.data(), got);
        if (m_state == SoundRecState::Recording) {
            m_pcmData.insert(m_pcmData.end(), buf.begin(), buf.begin() + got);
            m_recordSeconds += static_cast<double>(got) / static_cast<double>(m_sampleRate);
            updateTimerLabel();
        }
    } else {
#if defined(OMNISHELL_HAVE_ALSA)
        if (m_alsaPcm)
            wf->PushAmplitude(0.02f);
        else
#endif
            pushSimulatedMic(wf, m_synthPhase);
#if defined(OMNISHELL_HAVE_ALSA)
        if (m_state == SoundRecState::Recording && !m_alsaPcm) {
#else
        if (m_state == SoundRecState::Recording) {
#endif
            const int synthFrames =
                static_cast<int>(static_cast<double>(m_sampleRate) * static_cast<double>(kTickMs) / 1000.0);
            for (int i = 0; i < synthFrames; ++i) {
                m_synthPhase += 2.0 * 3.14159265358979323846 * 440.0 / static_cast<double>(m_sampleRate);
                const double s = std::sin(m_synthPhase) * 12000.0;
                m_pcmData.push_back(static_cast<int16_t>(std::clamp(s, -32768.0, 32767.0)));
            }
            m_recordSeconds += static_cast<double>(kTickMs) / 1000.0;
            updateTimerLabel();
        }
    }
}

} // namespace os

#include "StopWatchBody.hpp"

#include "../../core/App.hpp"

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/sizer.h>

namespace os {

namespace {
enum {
    ID_GROUP_SW = uiFrame::ID_APP_HIGHEST + 50,
    ID_START,
    ID_STOP,
    ID_RESET,
};
}

StopWatchBody::StopWatchBody() {
    auto theme = os::app.getIconTheme();

    group(ID_GROUP_SW, "controls", "stopwatch", 1000, "&StopWatch", "Stopwatch controls").install();

    int seq = 0;
    action(ID_START, "controls/stopwatch", "start", seq++, "&Start", "Start timer")
        .icon(theme->icon("stopwatch", "start"))
        .performFn([this](PerformContext* ctx) { onStart(ctx); })
        .install();
    action(ID_STOP, "controls/stopwatch", "stop", seq++, "S&top", "Stop timer")
        .icon(theme->icon("stopwatch", "stop"))
        .performFn([this](PerformContext* ctx) { onStop(ctx); })
        .install();
    action(ID_RESET, "controls/stopwatch", "reset", seq++, "&Reset", "Reset timer")
        .icon(theme->icon("stopwatch", "reset"))
        .performFn([this](PerformContext* ctx) { onReset(ctx); })
        .install();
}

StopWatchBody::~StopWatchBody() {
    if (m_timer.IsRunning())
        m_timer.Stop();
}

wxWindow* StopWatchBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return nullptr;
    m_frame = frame;

    m_frame->Bind(wxEVT_CLOSE_WINDOW, &StopWatchBody::onClose, this);

    wxPanel* root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_display = new wxStaticText(root, wxID_ANY, "00:00.0", wxDefaultPosition, wxDefaultSize,
                                wxALIGN_CENTRE_HORIZONTAL);
    wxFont font(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    m_display->SetFont(font);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* startBtn = new wxButton(root, wxID_ANY, "Start");
    wxButton* stopBtn = new wxButton(root, wxID_ANY, "Stop");
    wxButton* resetBtn = new wxButton(root, wxID_ANY, "Reset");

    startBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        auto pc = toPerformContext(e);
        onStart(&pc);
    });
    stopBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        auto pc = toPerformContext(e);
        onStop(&pc);
    });
    resetBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        auto pc = toPerformContext(e);
        onReset(&pc);
    });

    m_timer.SetOwner(m_frame);
    m_frame->Bind(wxEVT_TIMER, &StopWatchBody::onTimer, this, m_timer.GetId());

    buttonSizer->Add(startBtn, 1, wxALL, 5);
    buttonSizer->Add(stopBtn, 1, wxALL, 5);
    buttonSizer->Add(resetBtn, 1, wxALL, 5);

    sizer->Add(m_display, 1, wxEXPAND | wxALL | wxALIGN_CENTER, 10);
    sizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    root->SetSizer(sizer);

    return root;
}

void StopWatchBody::onStart(PerformContext*) {
    if (!m_timer.IsRunning()) {
        m_timer.Start(100);
    }
}

void StopWatchBody::onStop(PerformContext*) {
    if (m_timer.IsRunning()) {
        m_timer.Stop();
    }
}

void StopWatchBody::onReset(PerformContext*) {
    m_elapsedMs = 0;
    if (m_display) {
        m_display->SetLabel("00:00.0");
    }
}

void StopWatchBody::onTimer(wxTimerEvent&) {
    if (!m_frame || !m_display) {
        if (m_timer.IsRunning())
            m_timer.Stop();
        return;
    }
    m_elapsedMs += 100;
    int totalSeconds = m_elapsedMs / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    int tenths = (m_elapsedMs % 1000) / 100;

    wxString text;
    text.Printf("%02d:%02d.%d", minutes, seconds, tenths);
    m_display->SetLabel(text);
}

void StopWatchBody::onClose(wxCloseEvent& event) {
    if (m_timer.IsRunning())
        m_timer.Stop();
    if (m_frame) {
        m_frame->Unbind(wxEVT_TIMER, &StopWatchBody::onTimer, this, m_timer.GetId());
        m_frame->Unbind(wxEVT_CLOSE_WINDOW, &StopWatchBody::onClose, this);
    }
    m_display = nullptr;
    m_frame = nullptr;
    event.Skip();
}

} // namespace os


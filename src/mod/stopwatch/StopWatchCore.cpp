#include "StopWatchCore.hpp"

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

StopWatchCore::StopWatchCore() {
    const std::string dir = "heroicons/normal";

    group(ID_GROUP_SW, "controls", "stopwatch", 1000, "&StopWatch", "Stopwatch controls").install();

    int seq = 0;
    action(ID_START, "controls/stopwatch", "start", seq++, "&Start", "Start timer")
        .icon(wxART_TIP, dir, "clock.svg")
        .performFn([this](PerformContext* ctx) { onStart(ctx); })
        .install();
    action(ID_STOP, "controls/stopwatch", "stop", seq++, "S&top", "Stop timer")
        .icon(wxART_TIP, dir, "stop.svg")
        .performFn([this](PerformContext* ctx) { onStop(ctx); })
        .install();
    action(ID_RESET, "controls/stopwatch", "reset", seq++, "&Reset", "Reset timer")
        .icon(wxART_TIP, dir, "arrow-path.svg")
        .performFn([this](PerformContext* ctx) { onReset(ctx); })
        .install();
}

StopWatchCore::~StopWatchCore() {
    if (timer_.IsRunning())
        timer_.Stop();
}

void StopWatchCore::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    frame_ = frame;

    frame_->Bind(wxEVT_CLOSE_WINDOW, &StopWatchCore::onClose, this);

    wxPanel* root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    display_ = new wxStaticText(root, wxID_ANY, "00:00.0", wxDefaultPosition, wxDefaultSize,
                                wxALIGN_CENTRE_HORIZONTAL);
    wxFont font(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    display_->SetFont(font);

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

    timer_.SetOwner(frame_);
    frame_->Bind(wxEVT_TIMER, &StopWatchCore::onTimer, this, timer_.GetId());

    buttonSizer->Add(startBtn, 1, wxALL, 5);
    buttonSizer->Add(stopBtn, 1, wxALL, 5);
    buttonSizer->Add(resetBtn, 1, wxALL, 5);

    sizer->Add(display_, 1, wxEXPAND | wxALL | wxALIGN_CENTER, 10);
    sizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    root->SetSizer(sizer);
}

wxEvtHandler* StopWatchCore::getEventHandler() {
    return frame_ ? frame_->GetEventHandler() : nullptr;
}

void StopWatchCore::onStart(PerformContext*) {
    if (!timer_.IsRunning()) {
        timer_.Start(100);
    }
}

void StopWatchCore::onStop(PerformContext*) {
    if (timer_.IsRunning()) {
        timer_.Stop();
    }
}

void StopWatchCore::onReset(PerformContext*) {
    elapsedMs_ = 0;
    if (display_) {
        display_->SetLabel("00:00.0");
    }
}

void StopWatchCore::onTimer(wxTimerEvent&) {
    if (!frame_ || !display_) {
        if (timer_.IsRunning())
            timer_.Stop();
        return;
    }
    elapsedMs_ += 100;
    int totalSeconds = elapsedMs_ / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    int tenths = (elapsedMs_ % 1000) / 100;

    wxString text;
    text.Printf("%02d:%02d.%d", minutes, seconds, tenths);
    display_->SetLabel(text);
}

void StopWatchCore::onClose(wxCloseEvent& event) {
    if (timer_.IsRunning())
        timer_.Stop();
    if (frame_) {
        frame_->Unbind(wxEVT_TIMER, &StopWatchCore::onTimer, this, timer_.GetId());
        frame_->Unbind(wxEVT_CLOSE_WINDOW, &StopWatchCore::onClose, this);
    }
    display_ = nullptr;
    frame_ = nullptr;
    event.Skip();
}

} // namespace os


#ifndef OMNISHELL_MOD_STOPWATCH_CORE_HPP
#define OMNISHELL_MOD_STOPWATCH_CORE_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/stattext.h>
#include <wx/timer.h>

namespace os {

class StopWatchCore : public UIFragment {
public:
    StopWatchCore();
    ~StopWatchCore() override;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

private:
    uiFrame* frame_{nullptr};
    wxStaticText* display_{nullptr};
    wxTimer timer_;
    int elapsedMs_{0};

    void onStart(PerformContext* ctx);
    void onStop(PerformContext* ctx);
    void onReset(PerformContext* ctx);
    void onTimer(wxTimerEvent& event);
    void onClose(wxCloseEvent& event);
};

} // namespace os

#endif


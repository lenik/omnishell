#ifndef OMNISHELL_MOD_STOPWATCH_CORE_HPP
#define OMNISHELL_MOD_STOPWATCH_CORE_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/stattext.h>
#include <wx/timer.h>

namespace os {

class StopWatchBody : public UIFragment {
public:
    StopWatchBody();
    ~StopWatchBody() override;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

private:
    uiFrame* m_frame{nullptr};
    wxStaticText* m_display{nullptr};
    wxTimer m_timer;
    int m_elapsedMs{0};

    void onStart(PerformContext* ctx);
    void onStop(PerformContext* ctx);
    void onReset(PerformContext* ctx);
    void onTimer(wxTimerEvent& event);
    void onClose(wxCloseEvent& event);
};

} // namespace os

#endif


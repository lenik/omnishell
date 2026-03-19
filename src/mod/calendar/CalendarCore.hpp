#ifndef OMNISHELL_MOD_CALENDAR_CORE_HPP
#define OMNISHELL_MOD_CALENDAR_CORE_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/calctrl.h>

namespace os {

class CalendarCore : public UIFragment {
public:
    CalendarCore();
    ~CalendarCore() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

private:
    uiFrame* frame_{nullptr};
    wxCalendarCtrl* cal_{nullptr};

    void onToday(PerformContext* ctx);
    void onPrevMonth(PerformContext* ctx);
    void onNextMonth(PerformContext* ctx);
};

} // namespace os

#endif


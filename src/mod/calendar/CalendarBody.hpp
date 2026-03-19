#ifndef OMNISHELL_MOD_CALENDAR_CORE_HPP
#define OMNISHELL_MOD_CALENDAR_CORE_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/calctrl.h>

namespace os {

class CalendarBody : public UIFragment {
public:
    CalendarBody();
    ~CalendarBody() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

private:
    uiFrame* m_frame{nullptr};
    wxCalendarCtrl* m_cal{nullptr};

    void onToday(PerformContext* ctx);
    void onPrevMonth(PerformContext* ctx);
    void onNextMonth(PerformContext* ctx);
};

} // namespace os

#endif


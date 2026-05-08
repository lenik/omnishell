#include "CalendarBody.hpp"

#include "../../core/App.hpp"

#include <wx/dateevt.h>

namespace os {

namespace {
enum {
    ID_TODAY = uiFrame::ID_APP_HIGHEST + 1,
    ID_PREV_MONTH,
    ID_NEXT_MONTH,
    ID_GROUP_NAV = uiFrame::ID_APP_HIGHEST + 50,
};
}

CalendarBody::CalendarBody() {
    auto theme = os::app.getIconTheme();

    group(ID_GROUP_NAV, "view", "navigate", 1000, "&Navigate", "Calendar navigation").install();

    int seq = 0;
    action(ID_TODAY, "view/navigate", "today", seq++, "&Today", "Go to today")
        .icon(theme->icon("calendar", "today"))
        .performFn([this](PerformContext* ctx) { onToday(ctx); })
        .install();
    action(ID_PREV_MONTH, "view/navigate", "prev_month", seq++, "&Previous Month", "Go to previous month")
        .icon(theme->icon("calendar", "prev_month"))
        .performFn([this](PerformContext* ctx) { onPrevMonth(ctx); })
        .install();
    action(ID_NEXT_MONTH, "view/navigate", "next_month", seq++, "&Next Month", "Go to next month")
        .icon(theme->icon("calendar", "next_month"))
        .performFn([this](PerformContext* ctx) { onNextMonth(ctx); })
        .install();
}

wxWindow* CalendarBody::createFragmentView(CreateViewContext* ctx) {
    m_frame = ctx->findParentFrame();
    
    wxWindow* parent = ctx->getParent();
    m_cal = new wxCalendarCtrl(parent, wxID_ANY, wxDefaultDateTime, ctx->getPos(), ctx->getSize());
    return m_cal;
}

void CalendarBody::onToday(PerformContext*) {
    if (m_cal)
        m_cal->SetDate(wxDateTime::Today());
}

void CalendarBody::onPrevMonth(PerformContext*) {
    if (!m_cal)
        return;
    wxDateTime d = m_cal->GetDate();
    d.SetMonth((wxDateTime::Month)((int)d.GetMonth() - 1));
    m_cal->SetDate(d);
}

void CalendarBody::onNextMonth(PerformContext*) {
    if (!m_cal)
        return;
    wxDateTime d = m_cal->GetDate();
    d.SetMonth((wxDateTime::Month)((int)d.GetMonth() + 1));
    m_cal->SetDate(d);
}

} // namespace os


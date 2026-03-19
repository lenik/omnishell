#include "CalendarCore.hpp"

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

CalendarCore::CalendarCore() {
    std::string dir = "heroicons/normal";

    group(ID_GROUP_NAV, "view", "navigate", 1000, "&Navigate", "Calendar navigation").install();

    int seq = 0;
    action(ID_TODAY, "view/navigate", "today", seq++, "&Today", "Go to today")
        .icon(wxART_GO_HOME, dir, "calendar-days.svg")
        .performFn([this](PerformContext* ctx) { onToday(ctx); })
        .install();
    action(ID_PREV_MONTH, "view/navigate", "prev_month", seq++, "&Previous Month", "Go to previous month")
        .icon(wxART_GO_BACK, dir, "arrow-left.svg")
        .performFn([this](PerformContext* ctx) { onPrevMonth(ctx); })
        .install();
    action(ID_NEXT_MONTH, "view/navigate", "next_month", seq++, "&Next Month", "Go to next month")
        .icon(wxART_GO_FORWARD, dir, "arrow-right.svg")
        .performFn([this](PerformContext* ctx) { onNextMonth(ctx); })
        .install();
}

void CalendarCore::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    m_cal = new wxCalendarCtrl(parent, wxID_ANY, wxDefaultDateTime, ctx->getPos(), ctx->getSize());
}

wxEvtHandler* CalendarCore::getEventHandler() {
    return m_cal ? m_cal->GetEventHandler() : nullptr;
}

void CalendarCore::onToday(PerformContext*) {
    if (m_cal)
        m_cal->SetDate(wxDateTime::Today());
}

void CalendarCore::onPrevMonth(PerformContext*) {
    if (!m_cal)
        return;
    wxDateTime d = m_cal->GetDate();
    d.SetMonth((wxDateTime::Month)((int)d.GetMonth() - 1));
    m_cal->SetDate(d);
}

void CalendarCore::onNextMonth(PerformContext*) {
    if (!m_cal)
        return;
    wxDateTime d = m_cal->GetDate();
    d.SetMonth((wxDateTime::Month)((int)d.GetMonth() + 1));
    m_cal->SetDate(d);
}

} // namespace os


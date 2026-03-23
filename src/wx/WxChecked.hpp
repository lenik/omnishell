#ifndef OMNISHELL_WX_WX_CHECKED_HPP
#define OMNISHELL_WX_WX_CHECKED_HPP

#include <wx/string.h>

#include <exception>
#include <utility>

class wxEvtHandler;
class wxEvent;

namespace os {

/** Print message + stack trace to stdout (no GUI dialog). */
void wxCheckedReportStdEx(const wxString& context, const std::exception& e);
void wxCheckedReportUnknownEx(const wxString& context);
wxString wxCheckedStackTrace(unsigned skip_frames = 1);

/**
 * Build context as "ancestor/…/leaf · event_kind":
 * - Path: walk from event source (or sink) to root; each segment prefers wxWindow::GetName(), else
 *   top-level title / control label (slugified), else a short role (frame, panel, button, list, …).
 * - Event: maps wx event type to a short name (e.g. enter, left_up, button, list_item_selected).
 * With no wxWindow and ev==nullptr: "HandlerClass · callback".
 */
wxString wxInferHandlerContext(wxEvtHandler* sink, const wxEvent* ev);

template <typename F>
void wxInvokeChecked(wxEvtHandler* sink, const wxEvent* ev, F&& f) {
    wxString ctx = wxInferHandlerContext(sink, ev);
    try {
        std::forward<F>(f)();
    } catch (const std::exception& e) {
        wxCheckedReportStdEx(ctx, e);
    } catch (...) {
        wxCheckedReportUnknownEx(ctx);
    }
}

} // namespace os

#endif

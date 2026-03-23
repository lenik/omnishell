#ifndef OMNISHELL_WX_WXC_WINDOW_HPP
#define OMNISHELL_WX_WXC_WINDOW_HPP

#include "WxChecked.hpp"

#include <wx/wx.h>

namespace os {

/**
 * Checked wx windows: Bind() wraps handlers with wxInvokeChecked(sink, &event, …) so context is
 * inferred (UI path from window names/labels/roles + event kind). Use wxcBind() for other targets.
 *
 * Unbind() cannot match wrapped functors (different identity).
 */
template <typename WxBase>
class wxcMixin : public WxBase {
public:
    using WxBase::WxBase;

public:
#ifdef wxHAS_EVENT_BIND
    template <typename EventTag, typename Functor>
    void Bind(const EventTag& eventType, Functor functor, int winid = wxID_ANY, int lastId = wxID_ANY,
              wxObject* userData = NULL) {
        using EventArg = typename EventTag::EventClass;
        wxEvtHandler::Bind(eventType,
                           [this, functor](EventArg& evt) {
                               wxInvokeChecked(this, &evt, [&] { functor(evt); });
                           },
                           winid, lastId, userData);
    }

    template <typename EventTag, typename Class, typename EventArg, typename EventHandler>
    void Bind(const EventTag& eventType, void (Class::*method)(EventArg&), EventHandler* handler,
              int winid = wxID_ANY, int lastId = wxID_ANY, wxObject* userData = NULL) {
        wxEvtHandler::Bind(eventType,
                           [this, method, handler](EventArg& evt) {
                               wxInvokeChecked(this, &evt,
                                               [&] { (handler->*method)(evt); });
                           },
                           winid, lastId, userData);
    }

    template <typename EventTag, typename EventArg>
    void Bind(const EventTag& eventType, void (*function)(EventArg&), int winid = wxID_ANY,
              int lastId = wxID_ANY, wxObject* userData = NULL) {
        wxEvtHandler::Bind(eventType,
                           [this, function](EventArg& evt) {
                               wxInvokeChecked(this, &evt, [&] { function(evt); });
                           },
                           winid, lastId, userData);
    }
#endif
};

class wxcPanel : public wxcMixin<wxPanel> {
public:
    using wxcMixin<wxPanel>::wxcMixin;
};

class wxcFrame : public wxcMixin<wxFrame> {
public:
    using wxcMixin<wxFrame>::wxcMixin;
};

class wxcDialog : public wxcMixin<wxDialog> {
public:
    using wxcMixin<wxDialog>::wxcMixin;
};

#ifdef wxHAS_EVENT_BIND
template <typename EventTag, typename Functor>
void wxcBind(wxEvtHandler& eh, const EventTag& eventType, Functor functor, int winid = wxID_ANY,
             int lastId = wxID_ANY, wxObject* userData = NULL) {
    using EventArg = typename EventTag::EventClass;
    eh.wxEvtHandler::Bind(eventType,
                          [&eh, functor](EventArg& evt) {
                              wxInvokeChecked(&eh, &evt, [&] { functor(evt); });
                          },
                          winid, lastId, userData);
}

template <typename EventTag, typename Class, typename EventArg, typename EventHandler>
void wxcBind(wxEvtHandler& eh, const EventTag& eventType, void (Class::*method)(EventArg&),
             EventHandler* handler, int winid = wxID_ANY, int lastId = wxID_ANY,
             wxObject* userData = NULL) {
    eh.wxEvtHandler::Bind(eventType,
                          [&eh, method, handler](EventArg& evt) {
                              wxInvokeChecked(&eh, &evt, [&] { (handler->*method)(evt); });
                          },
                          winid, lastId, userData);
}

template <typename EventTag, typename EventArg>
void wxcBind(wxEvtHandler& eh, const EventTag& eventType, void (*function)(EventArg&),
             int winid = wxID_ANY, int lastId = wxID_ANY, wxObject* userData = NULL) {
    eh.wxEvtHandler::Bind(eventType,
                          [&eh, function](EventArg& evt) {
                              wxInvokeChecked(&eh, &evt, [&] { function(evt); });
                          },
                          winid, lastId, userData);
}
#endif

} // namespace os

#endif

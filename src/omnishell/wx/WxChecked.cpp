#include "WxChecked.hpp"

#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/intl.h>
#include <wx/listbase.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/statbmp.h>
#include <wx/statusbr.h>
#include <wx/taskbar.h>
#include <wx/textctrl.h>
#include <wx/toolbar.h>
#include <wx/toplevel.h>
#include <wx/treebase.h>
#include <wx/wx.h>

#include <iostream>

#if defined(__linux__) && defined(__GLIBC__)
#include <cxxabi.h>
#include <execinfo.h>
#endif

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace os {

namespace {

#if defined(__linux__) && defined(__GLIBC__)
wxString DemangleBacktraceLine(const char* symLine) {
    if (!symLine)
        return wxString();
    const char* lparen = std::strchr(symLine, '(');
    const char* plus = lparen ? std::strchr(lparen + 1, '+') : nullptr;
    const char* rparen = lparen ? std::strchr(lparen + 1, ')') : nullptr;
    if (lparen && plus && rparen && plus < rparen) {
        std::string m(lparen + 1, static_cast<size_t>(plus - (lparen + 1)));
        int st = 0;
        char* dem = abi::__cxa_demangle(m.c_str(), nullptr, nullptr, &st);
        if (dem) {
            wxString nice = wxString::FromUTF8(dem);
            std::free(dem);
            return nice + wxString(" | ") + wxString::FromUTF8(symLine);
        }
    }
    return wxString::FromUTF8(symLine);
}
#endif

wxString SlugifyLabel(wxString s) {
    wxString out;
    for (size_t i = 0; i < s.length(); ++i) {
        wxUniChar c = s[i];
        if (c == '/' || c == '\\')
            out += '_';
        else if (wxIsspace(c))
            out += '_';
        else if (c == '_' || c == '-' || c == '.')
            out += c;
        else if (c < 128 && (std::isalnum(static_cast<unsigned char>(c))))
            out += wxString(static_cast<wxUniChar>(std::tolower(static_cast<unsigned char>(c))));
    }
    while (out.Replace(wxT("__"), wxT("_")))
        ;
    out.Trim(false).Trim(true);
    if (out.length() > 48)
        out = out.Mid(0, 48);
    if (out.IsEmpty())
        return wxString(wxT("item"));
    return out;
}

wxString RoleFromWindow(wxWindow* w) {
    if (!w)
        return wxString();
    if (dynamic_cast<wxFrame*>(w))
        return wxString(wxT("frame"));
    if (dynamic_cast<wxDialog*>(w))
        return wxString(wxT("dialog"));
    if (dynamic_cast<wxMenuBar*>(w))
        return wxString(wxT("menubar"));
    if (dynamic_cast<wxToolBar*>(w))
        return wxString(wxT("toolbar"));
    if (dynamic_cast<wxStatusBar*>(w))
        return wxString(wxT("statusbar"));
    if (dynamic_cast<wxTextCtrl*>(w))
        return wxString(wxT("text"));
    if (dynamic_cast<wxButton*>(w))
        return wxString(wxT("button"));
    if (dynamic_cast<wxListCtrl*>(w))
        return wxString(wxT("list"));
    if (dynamic_cast<wxComboBox*>(w))
        return wxString(wxT("combobox"));
    if (dynamic_cast<wxChoice*>(w))
        return wxString(wxT("choice"));
    if (dynamic_cast<wxStaticBitmap*>(w))
        return wxString(wxT("bitmap"));
    if (dynamic_cast<wxStaticText*>(w))
        return wxString(wxT("label"));
    if (dynamic_cast<wxPanel*>(w))
        return wxString(wxT("panel"));
    if (w->GetClassInfo())
        return wxString(w->GetClassInfo()->GetClassName()).Mid(2).MakeLower();
    return wxString(wxT("window"));
}

wxString SegmentForWindow(wxWindow* w) {
    if (!w)
        return wxString();
    wxString name = w->GetName();
    if (!name.IsEmpty())
        return name;

    if (wxTopLevelWindow* tlw = dynamic_cast<wxTopLevelWindow*>(w)) {
        wxString t = SlugifyLabel(tlw->GetTitle());
        if (!t.IsEmpty() && t != wxT("item"))
            return t;
    }

    if (wxControl* c = dynamic_cast<wxControl*>(w)) {
        wxString lbl = c->GetLabel();
        if (!lbl.IsEmpty())
            return SlugifyLabel(lbl);
    }

    return RoleFromWindow(w);
}

wxString BuildUiPath(wxWindow* leaf) {
    if (!leaf)
        return wxString();
    std::vector<wxString> segs;
    for (wxWindow* w = leaf; w; w = w->GetParent()) {
        wxString seg = SegmentForWindow(w);
        if (!seg.IsEmpty())
            segs.push_back(seg);
    }
    if (segs.empty())
        return wxString();
    wxString path;
    for (auto it = segs.rbegin(); it != segs.rend(); ++it) {
        if (!path.IsEmpty())
            path += wxT('/');
        path += *it;
    }
    return path;
}

wxString WxEventKind(const wxEvent& e) {
    const wxEventType t = e.GetEventType();
#define EVTAG(evt, name)                                                                                               \
    if (t == (evt))                                                                                                    \
        return wxString(wxT(name))
    EVTAG(wxEVT_LEFT_DOWN, "left_down");
    EVTAG(wxEVT_LEFT_UP, "left_up");
    EVTAG(wxEVT_LEFT_DCLICK, "left_dclick");
    EVTAG(wxEVT_RIGHT_DOWN, "right_down");
    EVTAG(wxEVT_RIGHT_UP, "right_up");
    EVTAG(wxEVT_MOTION, "motion");
    EVTAG(wxEVT_ENTER_WINDOW, "enter");
    EVTAG(wxEVT_LEAVE_WINDOW, "leave");
    EVTAG(wxEVT_MOUSEWHEEL, "wheel");
    EVTAG(wxEVT_PAINT, "paint");
    EVTAG(wxEVT_ERASE_BACKGROUND, "erase_bg");
    EVTAG(wxEVT_SIZE, "size");
    EVTAG(wxEVT_SET_FOCUS, "focus");
    EVTAG(wxEVT_KILL_FOCUS, "kill_focus");
    EVTAG(wxEVT_KEY_DOWN, "key_down");
    EVTAG(wxEVT_KEY_UP, "key_up");
    EVTAG(wxEVT_CHAR, "char");
    EVTAG(wxEVT_CHAR_HOOK, "char_hook");
    EVTAG(wxEVT_TEXT, "text");
    EVTAG(wxEVT_TEXT_ENTER, "text_enter");
    EVTAG(wxEVT_TEXT_MAXLEN, "text_maxlen");
    EVTAG(wxEVT_COMMAND_BUTTON_CLICKED, "button");
    EVTAG(wxEVT_MENU, "menu");
    EVTAG(wxEVT_COMMAND_MENU_SELECTED, "menu");
    EVTAG(wxEVT_TIMER, "timer");
    EVTAG(wxEVT_ACTIVATE, "activate");
    EVTAG(wxEVT_ICONIZE, "iconize");
    EVTAG(wxEVT_CLOSE_WINDOW, "close_window");
    EVTAG(wxEVT_TASKBAR_LEFT_DOWN, "taskbar_left_down");
    EVTAG(wxEVT_TASKBAR_LEFT_UP, "taskbar_left_up");
    EVTAG(wxEVT_TASKBAR_RIGHT_DOWN, "taskbar_right_down");
    EVTAG(wxEVT_TASKBAR_RIGHT_UP, "taskbar_right_up");
    EVTAG(wxEVT_TASKBAR_LEFT_DCLICK, "taskbar_left_dclick");
    EVTAG(wxEVT_TASKBAR_RIGHT_DCLICK, "taskbar_right_dclick");
    EVTAG(wxEVT_TASKBAR_MOVE, "taskbar_move");
    EVTAG(wxEVT_LIST_ITEM_SELECTED, "list_item_selected");
    EVTAG(wxEVT_LIST_ITEM_ACTIVATED, "list_item_activated");
    EVTAG(wxEVT_LIST_ITEM_RIGHT_CLICK, "list_item_right_click");
    EVTAG(wxEVT_LIST_COL_CLICK, "list_col_click");
    EVTAG(wxEVT_TREE_SEL_CHANGED, "tree_sel_changed");
    EVTAG(wxEVT_TREE_ITEM_ACTIVATED, "tree_item_activated");
    EVTAG(wxEVT_TREE_ITEM_EXPANDING, "tree_item_expanding");
    EVTAG(wxEVT_TREE_ITEM_RIGHT_CLICK, "tree_item_right_click");
    EVTAG(wxEVT_COMBOBOX, "combobox");
#undef EVTAG
    return wxString::Format(wxT("event_%d"), static_cast<int>(t));
}

wxString SinkClassLabel(wxEvtHandler* sink) {
    if (!sink || !sink->GetClassInfo())
        return wxString(wxT("handler"));
    return wxString(sink->GetClassInfo()->GetClassName());
}

} // namespace

wxString wxInferHandlerContext(wxEvtHandler* sink, const wxEvent* ev) {
    wxString evPart = ev ? WxEventKind(*ev) : wxString(wxT("callback"));

    wxWindow* win = nullptr;
    if (ev) {
        wxObject* obj = ev->GetEventObject();
        win = wxDynamicCast(obj, wxWindow);
    }
    if (!win)
        win = wxDynamicCast(sink, wxWindow);

    wxString path = BuildUiPath(win);
    if (!path.IsEmpty())
        return path + wxT(" · ") + evPart;
    wxString who = SinkClassLabel(sink);
    return who + wxT(" · ") + evPart;
}

wxString wxCheckedStackTrace(unsigned skip_frames) {
#if defined(__linux__) && defined(__GLIBC__)
    void* buffer[48];
    int n = backtrace(buffer, 48);
    char** symbols = backtrace_symbols(buffer, n);
    if (!symbols)
        return wxString();

    wxString out;
    for (int i = static_cast<int>(skip_frames); i < n; ++i)
        out << DemangleBacktraceLine(symbols[i]) << '\n';
    std::free(symbols);
    return out;
#else
    (void)skip_frames;
    return wxString(wxT("(stack trace not available on this platform)\n"));
#endif
}

void wxCheckedReportStdEx(const wxString& context, const std::exception& e) {
    wxString brief =
        wxString::Format(_("Unhandled exception in %s\n\n%s"), context.c_str(), e.what());
    wxString stack = wxCheckedStackTrace(2);
    std::cout << brief.ToUTF8().data() << "\n--- stack ---\n" << stack.ToUTF8().data() << std::endl;
}

void wxCheckedReportUnknownEx(const wxString& context) {
    wxString brief = wxString::Format(_("Unknown exception in %s"), context.c_str());
    wxString stack = wxCheckedStackTrace(2);
    std::cout << brief.ToUTF8().data() << "\n--- stack ---\n" << stack.ToUTF8().data() << std::endl;
}

} // namespace os

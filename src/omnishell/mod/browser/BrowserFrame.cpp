#include "BrowserFrame.hpp"

#include <wx/accel.h>

namespace os {

namespace {

enum BrowserAccelId {
    ID_BROWSER_ACC_BACK = wxID_HIGHEST + 700,
    ID_BROWSER_ACC_FWD,
    ID_BROWSER_ACC_REFRESH,
    ID_BROWSER_ACC_FOCUS_URL,
};

} // namespace

BrowserFrame::BrowserFrame(VolumeManager* vm, std::string title)
    : uiFrame(std::move(title))
    , m_body(vm) {
    addFragment(&m_body);
    createView();

    wxAcceleratorEntry entries[5];
    entries[0].Set(wxACCEL_ALT, WXK_LEFT, ID_BROWSER_ACC_BACK);
    entries[1].Set(wxACCEL_ALT, WXK_RIGHT, ID_BROWSER_ACC_FWD);
    entries[2].Set(wxACCEL_NORMAL, WXK_F5, ID_BROWSER_ACC_REFRESH);
    entries[3].Set(wxACCEL_ALT, static_cast<int>('D'), ID_BROWSER_ACC_FOCUS_URL);
    entries[4].Set(wxACCEL_CTRL, static_cast<int>('L'), ID_BROWSER_ACC_FOCUS_URL);
    SetAcceleratorTable(wxAcceleratorTable(5, entries));

    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_body.goBack(); }, ID_BROWSER_ACC_BACK);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_body.goForward(); }, ID_BROWSER_ACC_FWD);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_body.refreshPage(); }, ID_BROWSER_ACC_REFRESH);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_body.focusAddressBar(); }, ID_BROWSER_ACC_FOCUS_URL);
}

wxWindow* BrowserFrame::createFragmentView(CreateViewContext* ctx) {
    return uiFrame::createFragmentView(ctx);
}

} // namespace os

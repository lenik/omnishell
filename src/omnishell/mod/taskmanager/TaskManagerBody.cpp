#include "TaskManagerBody.hpp"

#include "../../shell/Shell.hpp"

#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/toplevel.h>
#include <wx/window.h>

#include <cstdint>

namespace os {

namespace {
enum {
    ID_REFRESH = wxID_HIGHEST + 1,
    ID_END_TASK
};
}

TaskManagerBody::~TaskManagerBody() {
    if (m_refreshTimer.IsRunning())
        m_refreshTimer.Stop();
    if (m_frame)
        m_frame->Unbind(wxEVT_TIMER, &TaskManagerBody::OnTimer, this, m_refreshTimer.GetId());
}

void TaskManagerBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    m_panel = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    m_panel->SetMinSize(wxSize(600, 400));

    auto* mainsizer = new wxBoxSizer(wxVERTICAL);

    m_processList = new wxListCtrl(m_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxLC_REPORT | wxLC_SINGLE_SEL);
    m_processList->InsertColumn(0, "Window", wxLIST_FORMAT_LEFT, 360);
    m_processList->InsertColumn(1, "State", wxLIST_FORMAT_LEFT, 200);

    mainsizer->Add(m_processList, 1, wxEXPAND | wxALL, 10);

    auto* btnsizer = new wxBoxSizer(wxHORIZONTAL);
    m_refreshBtn = new wxButton(m_panel, ID_REFRESH, "Refresh");
    m_endTaskBtn = new wxButton(m_panel, ID_END_TASK, "End Task");
    btnsizer->Add(m_refreshBtn, 0, wxALL, 5);
    btnsizer->Add(m_endTaskBtn, 0, wxALL, 5);
    btnsizer->AddStretchSpacer();

    mainsizer->Add(btnsizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    m_panel->SetSizer(mainsizer);

    m_panel->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        if (e.GetId() == ID_REFRESH)
            OnRefresh(e);
        else if (e.GetId() == ID_END_TASK)
            OnEndTask(e);
    });

    m_refreshTimer.SetOwner(m_frame);
    m_frame->Bind(wxEVT_TIMER, &TaskManagerBody::OnTimer, this, m_refreshTimer.GetId());
    m_refreshTimer.Start(2000);

    refreshProcessList();
}

wxEvtHandler* TaskManagerBody::getEventHandler() {
    return m_frame ? m_frame->GetEventHandler() : (m_panel ? m_panel->GetEventHandler() : nullptr);
}

void TaskManagerBody::OnRefresh(wxCommandEvent& event) {
    (void)event;
    refreshProcessList();
}

void TaskManagerBody::OnEndTask(wxCommandEvent& event) {
    (void)event;
    long selected =
        m_processList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selected == -1) {
        wxMessageBox("Please select a window to close.", "Task Manager", wxOK | wxICON_INFORMATION);
        return;
    }

    const long pdata = m_processList->GetItemData(selected);
    auto* tlw = reinterpret_cast<wxTopLevelWindow*>(static_cast<intptr_t>(pdata));
    if (!tlw) {
        wxMessageBox("Invalid selection.", "Task Manager", wxOK | wxICON_ERROR);
        return;
    }

    wxWindow* dlgParent = m_frame ? static_cast<wxWindow*>(m_frame) : m_panel;
    wxString title = tlw->GetTitle();
    if (title.empty())
        title = "(untitled window)";

    if (wxMessageBox(wxString::Format("Close window \"%s\"?", title), "Confirm",
                     wxYES_NO | wxICON_WARNING, dlgParent) != wxYES)
        return;

    tlw->Close();
    refreshProcessList();
}

void TaskManagerBody::OnTimer(wxTimerEvent& event) {
    (void)event;
    refreshProcessList();
}

void TaskManagerBody::refreshProcessList() {
    m_processList->DeleteAllItems();

    ShellApp* shell = ShellApp::getInstance();
    wxWindow* shellMain = shell ? static_cast<wxWindow*>(shell->getMainWindow()) : nullptr;

    wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
    while (node) {
        wxWindow* w = node->GetData();
        node = node->GetNext();

        auto* tlw = dynamic_cast<wxTopLevelWindow*>(w);
        if (!tlw)
            continue;
        if (w == shellMain)
            continue;

        wxString title = tlw->GetTitle();
        if (title.empty())
            title = "(untitled)";

        wxString state;
        if (!tlw->IsShown())
            state = "Hidden";
        else if (tlw->IsIconized())
            state = "Minimized";
        else
            state = "Normal";

        long idx = m_processList->InsertItem(m_processList->GetItemCount(), title);
        m_processList->SetItem(idx, 1, state);
        m_processList->SetItemData(idx, reinterpret_cast<long>(tlw));
    }
}

} // namespace os

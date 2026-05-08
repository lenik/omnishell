#ifndef OMNISHELL_MOD_TASKMANAGER_BODY_HPP
#define OMNISHELL_MOD_TASKMANAGER_BODY_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/listctrl.h>
#include <wx/timer.h>

class wxButton;
class wxPanel;

namespace os {

class TaskManagerBody : public UIFragment {
public:
    TaskManagerBody() = default;
    ~TaskManagerBody() override;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

private:
    uiFrame* m_frame{nullptr};
    wxPanel* m_panel{nullptr};
    wxListCtrl* m_processList{nullptr};
    wxButton* m_refreshBtn{nullptr};
    wxButton* m_endTaskBtn{nullptr};

    wxTimer m_refreshTimer;

    void OnRefresh(wxCommandEvent& event);
    void OnEndTask(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    void refreshProcessList();
};

} // namespace os

#endif

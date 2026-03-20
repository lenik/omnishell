#ifndef OMNISHELL_SHELL_TASKBAR_HPP
#define OMNISHELL_SHELL_TASKBAR_HPP

#include "../core/Module.hpp"
#include "../core/Process.hpp"

#include <wx/taskbar.h>
#include <wx/wx.h>

#include <memory>
#include <vector>

namespace os {

/**
 * Taskbar Button
 * Represents a running application in the taskbar
 */
struct TaskbarButton {
    ModulePtr module;
    ProcessPtr process;
    wxWindow* window;
    wxButton* button;
    bool active;
    bool minimized;
    
    TaskbarButton()
        : window(nullptr)
        , button(nullptr)
        , active(false)
        , minimized(false)
    {}
};

/**
 * Taskbar
 * 
 * Bottom taskbar showing:
 * - Start button
 * - Running applications
 * - System tray area
 * - Clock
 * 
 * Features:
 * - Application switching
 * - Minimize/restore windows
 * - Active window indication
 */
class Taskbar : public wxPanel {
public:
    Taskbar(wxWindow* parent);
    virtual ~Taskbar();
    
    /**
     * Add a running application to taskbar
     */
    void addApplication(ModulePtr module, wxWindow* window);
    void addProcess(ProcessPtr process); // subscribes to window additions
    void removeProcess(ProcessPtr process);
    
    /**
     * Remove an application from taskbar
     */
    void removeApplication(ModulePtr module);
    
    /**
     * Update application state
     */
    void updateApplicationState(ModulePtr module, bool active, bool minimized);
    
    /**
     * Get taskbar height
     */
    static int GetDefaultHeight() { return 40; }
    
    /**
     * Set start menu (to show when start button clicked)
     */
    void setStartMenu(wxMenu* menu);
    
    /**
     * Get start control (custom painted; not wxButton so label stays white on GTK).
     */
    wxWindow* getStartButton() const { return m_startButton; }
    
    /**
     * Get application list panel
     */
    wxPanel* getAppListPanel() const { return m_appListPanel; }
    
    /**
     * Get tray panel
     */
    wxPanel* getTrayPanel() const { return m_trayPanel; }

protected:
    void OnStartButtonClick(wxCommandEvent& event);
    void OnAppButtonClick(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnPaint(wxPaintEvent& event);
    
    void UpdateButtonLayout();
    TaskbarButton* FindButtonByModule(ModulePtr module);
    TaskbarButton* FindButtonByWindow(wxWindow* window);
    void RemoveButtonByIndex(size_t idx);
    void SetButtonVisual(TaskbarButton& b);

private:
    DECLARE_EVENT_TABLE()
    
    wxWindow* m_startButton;
    wxPanel* m_appListPanel;
    wxPanel* m_trayPanel;
    wxStaticText* m_clock;
    
    std::vector<TaskbarButton> m_applications;
    wxMenu* m_startMenu;
    
    int m_buttonWidth;
    int m_buttonSpacing;
    int m_margin;
};

} // namespace os

#endif // OMNISHELL_SHELL_TASKBAR_HPP

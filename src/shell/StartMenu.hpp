#ifndef OMNISHELL_SHELL_START_MENU_HPP
#define OMNISHELL_SHELL_START_MENU_HPP

#include "../core/Module.hpp"

#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <functional>
#include <vector>

namespace os {

/**
 * Start Menu
 * 
 * Popup menu showing:
 * - Search box
 * - Module list
 * - Categories
 * - System tools
 * 
 * Features:
 * - Live search filtering
 * - Module launching
 * - Categorized items
 */
class StartMenu : public wxDialog {
public:
    StartMenu(wxWindow* parent);
    virtual ~StartMenu();
    
    /**
     * Show start menu at position
     */
    void ShowAt(const wxPoint& position);
    
    /**
     * Populate menu with modules
     */
    void populateModules(const std::vector<ModulePtr>& modules);
    
    /**
     * Clear module list
     */
    void clearModules();
    
    /**
     * Set launch callback
     */
    using LaunchCallback = std::function<void(ModulePtr)>;
    void setLaunchCallback(LaunchCallback callback);

protected:
    void OnSearch(wxCommandEvent& event);
    void OnModuleSelected(wxCommandEvent& event);
    void OnModuleDoubleClicked(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnClose(wxCloseEvent& event);

private:
    void FilterModules(const std::string& searchText);
    void CreateModuleList();
    
    wxTextCtrl* searchBox_;
    wxListBox* moduleList_;
    wxPanel* categoryPanel_;
    
    std::vector<ModulePtr> allModules_;
    std::vector<ModulePtr> filteredModules_;
    LaunchCallback launchCallback_;
    
    int searchHeight_;
    int itemHeight_;
    int menuWidth_;
    int menuHeight_;
};

} // namespace os

#endif // OMNISHELL_SHELL_START_MENU_HPP

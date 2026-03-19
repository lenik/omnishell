#ifndef OMNISHELL_APP_CONTROLPANEL_HPP
#define OMNISHELL_APP_CONTROLPANEL_HPP

#include "../../core/Module.hpp"

#include <wx/listctrl.h>
#include <wx/wx.h>

#include <vector>

namespace os {

/**
 * Control Panel Application Module
 * 
 * Configuration center with:
 * - Module manager
 * - User configuration
 * - System settings
 */
class ControlPanelApp : public Module {
public:
    explicit ControlPanelApp(CreateModuleContext* ctx);
    virtual ~ControlPanelApp();
    
    virtual ProcessPtr run() override;
    
    // Module metadata
    void initializeMetadata();

private:
    void createMainWindow();
    void createCategoryList();
    void createContentPanel();
    
    void OnCategorySelected(wxCommandEvent& event);
    void OnModuleDoubleClicked(wxCommandEvent& event);
    
    void showModuleManager();
    void showSystemSettings();
    void showUserConfiguration();
    void showDesktopSettings();
    void showDisplaySettings();
    void showAbout();
    
    wxFrame* frame_;
    wxListView* categoryList_;
    wxPanel* contentPanel_;
    
    struct Category {
        wxString name;
        wxString description;
        int imageIndex;
    };
    
    std::vector<Category> categories_;
};

} // namespace os

#endif // OMNISHELL_APP_CONTROLPANEL_HPP

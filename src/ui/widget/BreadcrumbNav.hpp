#ifndef BREADCRUMBNAV_H
#define BREADCRUMBNAV_H

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/wx.h>

#include <functional>
#include <string>
#include <vector>

/**
 * Breadcrumb navigation widget for displaying current path
 * Converted from QML BreadcrumbNav to wxWidgets
 */
class BreadcrumbNav : public wxPanel {
private:
    std::string m_location;
    std::vector<wxButton*> m_breadcrumbButtons;
    wxBoxSizer* m_sizer;
    
    // Callbacks
    std::vector<std::function<void(const std::string&)>> m_pathSelectedListeners;
    
    void updateBreadcrumbs();
    void clearBreadcrumbs();
    std::vector<std::string> splitPath(const std::string& path);
    
public:
    BreadcrumbNav(wxWindow* parent, const std::string& location);
    virtual ~BreadcrumbNav();
    
    // Path management
    void setLocation(const std::string& location);
    const std::string& getLocation() const { return m_location; }
    
    // Event callbacks
    void onPathSelected(std::function<void(const std::string&)> callback) {
        m_pathSelectedListeners.push_back(callback);
    }
    void notifyPathSelected(const std::string& path) {
        for (auto& listener : m_pathSelectedListeners) {
            listener(path);
        }
    }
    
private:
    // Event handlers
    void OnBreadcrumbClicked(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

#endif // BREADCRUMBNAV_H

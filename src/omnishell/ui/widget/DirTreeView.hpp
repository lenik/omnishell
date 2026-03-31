#ifndef DIRTREEVIEW_H
#define DIRTREEVIEW_H

#include "../Location.hpp"

#include <wx/treectrl.h>
#include <wx/wx.h>

#include <functional>

/**
 * File tree view widget for displaying directory structure
 * Converted from QML DirTreeView to wxWidgets
 */
class DirTreeView : public wxTreeCtrl {
private:
    Location m_location;
    
    std::vector<std::function<void(const Location& location)>> m_locationChangeListeners;
    
    // Tree item data
    struct TreeItemData : public wxTreeItemData {
        std::string path;
        bool isDirectory;
        
        TreeItemData(const std::string& p, bool isDir) : path(p), isDirectory(isDir) {}
    };
    
    void populateTree();
    void populateNode(const wxTreeItemId& parentItem, const std::string& path);
    wxTreeItemId findItemByPath(const std::string& path);
    
public:
    DirTreeView(wxWindow* parent, const Location& location);
    virtual ~DirTreeView();
    
    const Location& getLocation() const { return m_location; }
    void setLocation(const Location& location);

    void refresh();
    void expandPath(const std::string& path);
    
    void onLocationChange(std::function<void(const Location& location)> callback) {
        m_locationChangeListeners.push_back(callback);
    }
    void notifyLocationChange(const Location& location) {
        for (auto& listener : m_locationChangeListeners) {
            listener(location);
        }
    }
    
private:
    // Event handlers
    void OnSelectionChanged(wxTreeEvent& event);
    void OnItemActivated(wxTreeEvent& event);
    void OnItemExpanding(wxTreeEvent& event);
    void OnRightClick(wxTreeEvent& event);
    
    DECLARE_EVENT_TABLE()
};

#endif // DIRTREEVIEW_H

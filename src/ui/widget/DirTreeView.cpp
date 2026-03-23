#include "DirTreeView.hpp"

#include "../../wx/WxChecked.hpp"

#include <bas/volume/Volume.hpp>

#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/treectrl.h>

wxBEGIN_EVENT_TABLE(DirTreeView, wxTreeCtrl)
    EVT_TREE_SEL_CHANGED(wxID_ANY, DirTreeView::OnSelectionChanged)
    EVT_TREE_ITEM_ACTIVATED(wxID_ANY, DirTreeView::OnItemActivated)
    EVT_TREE_ITEM_EXPANDING(wxID_ANY, DirTreeView::OnItemExpanding)
    EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, DirTreeView::OnRightClick)
wxEND_EVENT_TABLE()

DirTreeView::DirTreeView(wxWindow* parent, const Location& location)
    : wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                 wxTR_DEFAULT_STYLE | wxTR_SINGLE | wxTR_LINES_AT_ROOT),
      m_location(location) {

    SetName(wxT("dir_tree"));
    populateTree();
}

DirTreeView::~DirTreeView() {
}

void DirTreeView::setLocation(const Location& location) {
    if (m_location == location)
        return;
    // Volume change: rebuild tree. Same volume: only sync selection to preserve expand state.
    if (m_location.volume != location.volume) {
        m_location = location;
        refresh();
        return;
    }
    m_location = location;
    // Expand path prefixes so findItemByPath can reach the target (e.g. /icon for /icon/png)
    std::string path = m_location.path;
    if (path.size() > 1) {
        size_t pos = 0;
        while ((pos = path.find('/', pos + 1)) != std::string::npos) {
            expandPath(path.substr(0, pos));
        }
        size_t lastSlash = path.rfind('/');
        if (lastSlash > 0)
            expandPath(path.substr(0, lastSlash));
    }
    wxTreeItemId item = findItemByPath(m_location.path);
    if (item.IsOk()) {
        SelectItem(item);
        EnsureVisible(item);
    }
}

void DirTreeView::refresh() {
    DeleteAllItems();
    populateTree();
    // Expand all path prefixes so findItemByPath can reach nested items (e.g. /icon for /icon/png)
    std::string path = m_location.path;
    if (path.size() > 1) {
        size_t pos = 0;
        while ((pos = path.find('/', pos + 1)) != std::string::npos) {
            expandPath(path.substr(0, pos));
        }
        size_t lastSlash = path.rfind('/');
        if (lastSlash > 0)
            expandPath(path.substr(0, lastSlash));
    }
    wxTreeItemId item = findItemByPath(m_location.path);
    if (item.IsOk()) {
        SelectItem(item);
        EnsureVisible(item);
    }
}

void DirTreeView::expandPath(const std::string& path) {
    wxTreeItemId item = findItemByPath(path);
    if (item.IsOk()) {
        Expand(item);
    }
}

void DirTreeView::populateTree() {
    // Add root item
    wxTreeItemId rootItem = AddRoot("Root", -1, -1, new TreeItemData("/", true));
    
    // Populate root level
    populateNode(rootItem, "/");
    
    // Expand root
    Expand(rootItem);
}

void DirTreeView::populateNode(const wxTreeItemId& parentItem, const std::string& path) {
    if (!parentItem.IsOk()) return;
    
    if (m_location.volume == nullptr) {
        printf("m_location.volume nullptr !!\n");
    }
    
    try {
        auto entries = m_location.volume->readDir(path, false);
        
        for (const auto& entry : entries) {
            if (entry && entry->isDirectory()) {
                std::string fullPath = (path == "/") ? "/" + entry->name : path + "/" + entry->name;
                
                wxTreeItemId childItem = AppendItem(parentItem, entry->name, -1, -1,
                                                   new TreeItemData(fullPath, true));
                
                // Add a dummy child to make it expandable
                AppendItem(childItem, "Loading...", -1, -1, nullptr);
            }
        }
    } catch (const std::exception& e) {
        wxMessageBox("Error reading directory: " + std::string(e.what()), 
                     "Error", wxOK | wxICON_ERROR);
    }
}

wxTreeItemId DirTreeView::findItemByPath(const std::string& path) {
    wxTreeItemId rootItem = GetRootItem();
    if (!rootItem.IsOk()) return wxTreeItemId();
    
    if (path == "/") {
        return rootItem;
    }
    
    // Split path into components
    std::vector<std::string> components;
    std::string currentPath = path;
    
    // Remove leading slash
    if (currentPath.front() == '/') {
        currentPath = currentPath.substr(1);
    }
    
    size_t pos = 0;
    while ((pos = currentPath.find('/')) != std::string::npos) {
        components.push_back(currentPath.substr(0, pos));
        currentPath = currentPath.substr(pos + 1);
    }
    if (!currentPath.empty()) {
        components.push_back(currentPath);
    }
    
    // Navigate through the tree
    wxTreeItemId currentItem = rootItem;
    
    for (const std::string& component : components) {
        wxTreeItemIdValue cookie;
        wxTreeItemId childItem = GetFirstChild(currentItem, cookie);
        bool found = false;
        
        while (childItem.IsOk()) {
            if (GetItemText(childItem) == component) {
                currentItem = childItem;
                found = true;
                break;
            }
            childItem = GetNextChild(currentItem, cookie);
        }
        
        if (!found) {
            return wxTreeItemId(); // Path not found
        }
    }
    
    return currentItem;
}

void DirTreeView::OnSelectionChanged(wxTreeEvent& event) {
    os::wxInvokeChecked(this, &event, [&] {
        wxTreeItemId item = event.GetItem();
        if (item.IsOk()) {
            TreeItemData* data = dynamic_cast<TreeItemData*>(GetItemData(item));
            if (data) {
                notifyLocationChange(Location(m_location.volume, data->path));
            }
        }
    });
    event.Skip();
}

void DirTreeView::OnItemActivated(wxTreeEvent& event) {
    os::wxInvokeChecked(this, &event, [&] {
        wxTreeItemId item = event.GetItem();
        if (item.IsOk()) {
            TreeItemData* data = dynamic_cast<TreeItemData*>(GetItemData(item));
            if (data) {
                notifyLocationChange(Location(m_location.volume, data->path));
            }
        }
    });
    event.Skip();
}

void DirTreeView::OnItemExpanding(wxTreeEvent& event) {
    os::wxInvokeChecked(this, &event, [&] {
        wxTreeItemId item = event.GetItem();
        if (!item.IsOk())
            return;

        TreeItemData* data = dynamic_cast<TreeItemData*>(GetItemData(item));
        if (!data)
            return;

        wxTreeItemIdValue cookie;
        wxTreeItemId firstChild = GetFirstChild(item, cookie);

        if (firstChild.IsOk() && GetItemText(firstChild) == "Loading...") {
            Delete(firstChild);
            populateNode(item, data->path);
        }
    });
    event.Skip();
}

void DirTreeView::OnRightClick(wxTreeEvent& event) {
    os::wxInvokeChecked(this, &event, [&] {
        wxTreeItemId item = event.GetItem();
        if (!item.IsOk())
            return;

        TreeItemData* data = dynamic_cast<TreeItemData*>(GetItemData(item));
        if (!data)
            return;

        wxMenu contextMenu;
        contextMenu.Append(wxID_OPEN, "Open");
        contextMenu.Append(wxID_REFRESH, "Refresh");
        contextMenu.AppendSeparator();
        contextMenu.Append(wxID_PROPERTIES, "Properties");

        PopupMenu(&contextMenu);
    });
    event.Skip();
}

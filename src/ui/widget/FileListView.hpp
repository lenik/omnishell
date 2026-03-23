#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include "../Location.hpp"

#include <bas/volume/DirEntry.hpp>
#include <bas/volume/VolumeFile.hpp>

#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/wx.h>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * File list view widget for displaying directory contents
 * Converted from QML FileList to wxWidgets
 */
class FileListView : public wxListCtrl {
private:
    Location m_location;
    std::vector<DirEntry> m_entries;
    std::function<bool(const DirEntry&)> m_entryFilter;
    
    // View modes
    enum ViewMode {
        LIST_MODE,
        GRID_MODE
    };
    ViewMode m_viewMode;
    
    wxImageList* m_imageListSmall;
    wxImageList* m_imageListLarge;
    static const int ICON_SIZE_SMALL = 16;
    static const int ICON_SIZE_LARGE = 48;
    int m_sortColumn;
    bool m_sortAscending;
    static const size_t SMALL_IMAGE_THRESHOLD = 16 * 1024; // 16KB
    std::unordered_map<std::string, int> m_thumbnailCache; // path -> image list index (large list)
    
    // Callbacks for events
    std::vector<std::function<void(VolumeFile& file)>> m_fileSelectedListeners;
    std::vector<std::function<void(VolumeFile& file)>> m_fileActivatedListeners;
    std::vector<std::function<void(const std::vector<VolumeFile>& files)>> m_selectionChangedListeners;
    
    void setupColumns();
    void updateColumnHeaders();
    void setupImageLists();
    int getIconIndex(const DirEntry& entry, size_t listIndex);
    void populateList();
    /** Refresh list control from m_entries (no re-fetch). Used after sorting. */
    void refreshListFromEntries();
    void updateEntry(int index, const DirEntry& entry);
    std::string formatFileSize(uint64_t size);
    std::string formatDateTime(uint64_t timestamp);
    
public:
    FileListView(wxWindow* parent, const Location& location);
    virtual ~FileListView();
    
    // Navigation
    Location& getLocation() { return m_location; }
    void setLocation(const Location& location);

    void refresh();

    /** If set, entries failing the predicate are hidden (e.g. file type filters). Directories are often included by the predicate. */
    void setEntryFilter(std::function<bool(const DirEntry&)> filter);
    void clearEntryFilter();
    
    // View mode
    void setViewMode(const std::string& mode);
    std::string getViewMode() const;
    
    // Selection
    std::vector<std::string> getSelectedFiles() const;
    void selectFile(const std::string& filename);
    void selectAll();
    void clearSelection();
    
    // Sorting
    void sortByName(bool ascending = true);
    void sortBySize(bool ascending = true);
    void sortByDate(bool ascending = true);
    void sortByType(bool ascending = true);
    
    // Event callbacks
    void onFileSelected(std::function<void(VolumeFile& file)> callback) {
        m_fileSelectedListeners.emplace_back(callback);
    }
    void onFileActivated(std::function<void(VolumeFile& file)> callback) {
        m_fileActivatedListeners.emplace_back(callback);
    }
    void onSelectionChanged(std::function<void(const std::vector<VolumeFile>& files)> callback) {
        m_selectionChangedListeners.emplace_back(callback);
    }
    
    void notifyFileSelected(VolumeFile& file) {
        for (auto& listener : m_fileSelectedListeners) {
            listener(file);
        }
    }
    void notifyFileActivated(VolumeFile& file) {
        for (auto& listener : m_fileActivatedListeners) {
            listener(file);
        }
    }
    void notifySelectionChanged(const std::vector<VolumeFile>& files) {
        for (auto& listener : m_selectionChangedListeners) {
            listener(files);
        }
    }
    
private:
    // Event handlers
    void OnItemSelected(wxListEvent& event);
    void OnItemActivated(wxListEvent& event);
    void OnItemRightClick(wxListEvent& event);
    void OnColumnClick(wxListEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    
    bool m_rectSelecting;
    int m_rectStartItem;
    wxPoint m_rectStartPos;
    
    DECLARE_EVENT_TABLE()
};

#endif // FILELISTVIEW_H

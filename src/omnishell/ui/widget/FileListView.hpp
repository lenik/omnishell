#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include "../Location.hpp"

#include <bas/volume/DirEntry.hpp>
#include <bas/volume/VolumeFile.hpp>

#include <wx/image.h>
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
    // Thumbnail generation is gated by file size to avoid decoding huge images everywhere.
    // Cache holds decoded+scaled wxImage so we can derive faded variants without re-decoding.
    size_t m_thumbnailMaxBytes{16 * 1024}; // default 16KB (keeps old behaviour)
    std::unordered_map<std::string, wxImage> m_thumbnailBaseCache; // thumbKey(path,size) -> scaled wxImage
    std::unordered_map<std::string, int> m_thumbnailVariantCache; // variantKey -> image list index

    // Recency fade style (used by Camera gallery):
    // - show last `m_recencyRecentCount` items as "recent" with a highlight border
    // - fade older items based on distance from the recent window
    bool m_recencyFadeEnabled{false};
    size_t m_recencyRecentCount{0};
    size_t m_recencyFadeSteps{7}; // number of visual steps for older items
    bool m_recencyBorderEnabled{true};
    
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
    
    // Thumbnail / recency styling
    // - recentCount: last N items (after the list has been sorted by date) are shown "normal"
    // - older items fade out based on distance from the recent window
    void enableRecencyFade(size_t recentCount, size_t fadeSteps = 7, bool border = true);
    void disableRecencyFade();
    void setThumbnailMaxBytes(size_t maxBytes);
    
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

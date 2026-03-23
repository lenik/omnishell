#include "FileListView.hpp"
#include "wx/artprov.h"

#include <bas/ui/arch/ImageSet.hpp>
#include <bas/util/FileTypeDetector.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/wx/artprovs.hpp>
#include <bas/wx/images.hpp>

#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace {

std::string joinVolumePath(const std::string& base, const std::string& name) {
    if (base.empty() || base == "/")
        return "/" + name;
    return base + "/" + name;
}

} // namespace

wxBEGIN_EVENT_TABLE(FileListView, wxListCtrl)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, FileListView::OnItemSelected)
    EVT_LIST_ITEM_ACTIVATED(wxID_ANY, FileListView::OnItemActivated)
    EVT_LIST_ITEM_RIGHT_CLICK(wxID_ANY, FileListView::OnItemRightClick)
    EVT_LIST_COL_CLICK(wxID_ANY, FileListView::OnColumnClick)
    EVT_KEY_DOWN(FileListView::OnKeyDown)
    EVT_LEFT_DOWN(FileListView::OnLeftDown)
    EVT_LEFT_UP(FileListView::OnLeftUp)
    EVT_MOTION(FileListView::OnMotion)
wxEND_EVENT_TABLE()

static wxBitmap IconBitmapForList(const wxArtID& id, int size) {
    ImageSet image(id);
    return image.toBitmap1(size, size, wxART_LIST);
}

FileListView::FileListView(wxWindow* parent, const Location& location)
    : wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                 wxLC_REPORT | wxLC_SORT_ASCENDING),
      m_location(location), m_viewMode(LIST_MODE),
      m_imageListSmall(nullptr), m_imageListLarge(nullptr),
      m_sortColumn(0), m_sortAscending(true),
      m_rectSelecting(false), m_rectStartItem(-1) {
    
    setupImageLists();
    AssignImageList(m_imageListSmall, wxIMAGE_LIST_SMALL);
    AssignImageList(m_imageListLarge, wxIMAGE_LIST_NORMAL);
    
    setupColumns();
    
    populateList();
}

FileListView::~FileListView() {
    // Image lists are owned by wxListCtrl after AssignImageList() call
    // They will be deleted by the base class destructor
    m_imageListSmall = nullptr;
    m_imageListLarge = nullptr;
}

void FileListView::setupImageLists() {
    m_imageListSmall = new wxImageList(ICON_SIZE_SMALL, ICON_SIZE_SMALL);
    m_imageListLarge = new wxImageList(ICON_SIZE_LARGE, ICON_SIZE_LARGE);
    wxBitmap folderS = IconBitmapForList(wxT("wxART_FOLDER"), ICON_SIZE_SMALL);
    wxBitmap fileS = IconBitmapForList(wxT("wxART_NORMAL_FILE"), ICON_SIZE_SMALL);
    wxBitmap imageS = IconBitmapForList(wxT("wxART_MISSING_IMAGE"), ICON_SIZE_SMALL);
    wxBitmap folderL = IconBitmapForList(wxT("wxART_FOLDER"), ICON_SIZE_LARGE);
    wxBitmap fileL = IconBitmapForList(wxT("wxART_NORMAL_FILE"), ICON_SIZE_LARGE);
    wxBitmap imageL = IconBitmapForList(wxT("wxART_MISSING_IMAGE"), ICON_SIZE_LARGE);
    m_imageListSmall->Add(folderS);   // 0
    m_imageListSmall->Add(fileS);     // 1
    m_imageListSmall->Add(imageS);   // 2
    m_imageListLarge->Add(folderL);
    m_imageListLarge->Add(fileL);
    m_imageListLarge->Add(imageL);
}

int FileListView::getIconIndex(const DirEntry& entry, size_t listIndex) {
    const int size = (m_viewMode == LIST_MODE) ? ICON_SIZE_SMALL : ICON_SIZE_LARGE;
    wxImageList* imgList = (m_viewMode == LIST_MODE) ? m_imageListSmall : m_imageListLarge;
    if (entry.isDirectory()) return 0;
    using FT = FileTypeDetector::FileType;
    FT ft = FileTypeDetector::detectFileType(entry.name);
    if (ft == FT::IMAGE && entry.size > 0 && entry.size <= SMALL_IMAGE_THRESHOLD && m_location.volume) {
        std::string path = joinVolumePath(m_location.path, entry.name);
        auto it = m_thumbnailCache.find(path);
        if (it != m_thumbnailCache.end()) return it->second;
        try {
            VolumeFile vf(m_location.volume, path);
            std::vector<uint8_t> data = vf.readFile();
            if (!data.empty()) {
                wxMemoryInputStream stream(data.data(), data.size());
                wxImage img;
                if (img.LoadFile(stream, wxBITMAP_TYPE_ANY) && img.IsOk()) {
                    img = img.Scale(size, size, wxIMAGE_QUALITY_BILINEAR);
                    wxBitmap bmp(img);
                    if (bmp.IsOk()) {
                        int idx = imgList->Add(bmp);
                        m_thumbnailCache[path] = idx;
                        return idx;
                    }
                }
            }
        } catch (...) {}
    }
    if (ft == FT::IMAGE) return 2;
    return 1;
}

void FileListView::updateColumnHeaders() {
    if (m_viewMode != LIST_MODE || GetColumnCount() < 4) return;
    const char* base[] = { "Name", "Size", "Type", "Modified" };
    for (int c = 0; c < 4; ++c) {
        wxString label = base[c];
        if (c == m_sortColumn) label += m_sortAscending ? " \u25B2" : " \u25BC";
        wxListItem item;
        GetColumn(c, item);
        item.SetMask(wxLIST_MASK_TEXT);
        item.SetText(label);
        SetColumn(c, item);
    }
}

void FileListView::setLocation(const Location& location) {
    if (m_location == location)
        return;

    m_location = location;
    m_entries.clear();
    m_thumbnailCache.clear();
    DeleteAllItems();
    populateList();
}

void FileListView::refresh() {
    populateList();
}

void FileListView::setEntryFilter(std::function<bool(const DirEntry&)> filter) {
    m_entryFilter = std::move(filter);
}

void FileListView::clearEntryFilter() {
    m_entryFilter = nullptr;
}

void FileListView::setViewMode(const std::string& mode) {
    if (mode == "list") {
        m_viewMode = LIST_MODE;
        SetWindowStyleFlag(wxLC_REPORT | wxLC_SORT_ASCENDING);
    } else if (mode == "grid") {
        m_viewMode = GRID_MODE;
        SetWindowStyleFlag(wxLC_ICON);
    }
    
    setupColumns();
    populateList();
}

std::string FileListView::getViewMode() const {
    return (m_viewMode == LIST_MODE) ? "list" : "grid";
}

std::vector<std::string> FileListView::getSelectedFiles() const {
    std::vector<std::string> selectedFiles;
    
    long selectedIndex = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    while (selectedIndex != wxNOT_FOUND) {
        if (selectedIndex < static_cast<long>(m_entries.size())) {
            selectedFiles.push_back(m_entries[selectedIndex].name);
        }
        selectedIndex = GetNextItem(selectedIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }
    
    return selectedFiles;
}

void FileListView::selectFile(const std::string& filename) {
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].name == filename) {
            SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
            EnsureVisible(i);
            break;
        }
    }
}

void FileListView::selectAll() {
    for (long i = 0; i < GetItemCount(); ++i) {
        SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
}

void FileListView::clearSelection() {
    long selectedIndex = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    while (selectedIndex != wxNOT_FOUND) {
        SetItemState(selectedIndex, 0, wxLIST_STATE_SELECTED);
        selectedIndex = GetNextItem(selectedIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }
}

void FileListView::sortByName(bool ascending) {
    std::sort(m_entries.begin(), m_entries.end(), 
        [ascending](const DirEntry& a, const DirEntry& b) {
            if (a.isDirectory() != b.isDirectory()) {
                return a.isDirectory(); // Directories first
            }
            std::string aName = a.name;
            std::string bName = b.name;
            std::transform(aName.begin(), aName.end(), aName.begin(), ::tolower);
            std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
            return ascending ? (aName < bName) : (aName > bName);
        });
    
    refreshListFromEntries();
}

void FileListView::sortBySize(bool ascending) {
    std::sort(m_entries.begin(), m_entries.end(), 
        [ascending](const DirEntry& a, const DirEntry& b) {
            if (a.isDirectory() != b.isDirectory()) {
                return a.isDirectory(); // Directories first
            }
            return ascending ? (a.size < b.size) : (a.size > b.size);
        });
    
    refreshListFromEntries();
}

void FileListView::sortByDate(bool ascending) {
    std::sort(m_entries.begin(), m_entries.end(), 
        [ascending](const DirEntry& a, const DirEntry& b) {
            if (a.isDirectory() != b.isDirectory()) {
                return a.isDirectory(); // Directories first
            }
            return ascending ? (a.modifiedTime < b.modifiedTime) : (a.modifiedTime > b.modifiedTime);
        });
    
    refreshListFromEntries();
}

void FileListView::sortByType(bool ascending) {
    std::sort(m_entries.begin(), m_entries.end(), 
        [ascending](const DirEntry& a, const DirEntry& b) {
            if (a.isDirectory() != b.isDirectory()) {
                return a.isDirectory(); // Directories first
            }
            
            // Get file extensions for comparison
            auto getExtension = [](const std::string& name) {
                size_t dotPos = name.find_last_of('.');
                return (dotPos != std::string::npos) ? name.substr(dotPos + 1) : "";
            };
            
            std::string aExt = getExtension(a.name);
            std::string bExt = getExtension(b.name);
            std::transform(aExt.begin(), aExt.end(), aExt.begin(), ::tolower);
            std::transform(bExt.begin(), bExt.end(), bExt.begin(), ::tolower);
            
            return ascending ? (aExt < bExt) : (aExt > bExt);
        });
    
    refreshListFromEntries();
}

void FileListView::refreshListFromEntries() {
    DeleteAllItems();
    for (size_t i = 0; i < m_entries.size(); ++i) {
        updateEntry(static_cast<int>(i), m_entries[i]);
    }
}

void FileListView::setupColumns() {
    DeleteAllColumns();
    
    if (m_viewMode == LIST_MODE) {
        InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 200);
        InsertColumn(1, "Size", wxLIST_FORMAT_RIGHT, 80);
        InsertColumn(2, "Type", wxLIST_FORMAT_LEFT, 100);
        InsertColumn(3, "Modified", wxLIST_FORMAT_LEFT, 150);
        updateColumnHeaders();
    }
}

void FileListView::populateList() {
    DeleteAllItems();
    m_entries.clear();
    
    try {
        auto fileStatuses = m_location.volume->readDir(m_location.path, false);
        
        // Convert FileStatus pointers to DirEntry objects
        m_entries.clear();
        m_entries.reserve(fileStatuses.size());
        for (const auto& fs : fileStatuses) {
            if (fs) {
                m_entries.push_back(*fs); // FileStatus extends DirEntry, so we can copy
            }
        }

        if (m_entryFilter) {
            m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                                           [this](const DirEntry& e) { return !m_entryFilter(e); }),
                            m_entries.end());
        }
        
        // Sort entries by default (directories first, then alphabetically).
        // sortByName() calls refreshListFromEntries() which already populates the list.
        sortByName(true);
        
    } catch (const std::exception& e) {
        wxMessageBox("Error reading directory: " + std::string(e.what()), 
                     "Error", wxOK | wxICON_ERROR);
    }
}

void FileListView::updateEntry(int index, const DirEntry& entry) {
    int iconIndex = getIconIndex(entry, index);
    if (m_viewMode == LIST_MODE) {
        long itemIndex = InsertItem(index, wxString::FromUTF8(entry.name.c_str()), iconIndex);
        
        if (entry.isDirectory()) {
            SetItem(itemIndex, 1, "<DIR>");
        } else {
            SetItem(itemIndex, 1, formatFileSize(entry.size));
        }
        
        if (entry.isDirectory()) {
            SetItem(itemIndex, 2, "Folder");
        } else {
            size_t dotPos = entry.name.find_last_of('.');
            std::string extension = (dotPos != std::string::npos) ?
                                   entry.name.substr(dotPos + 1) : "File";
            SetItem(itemIndex, 2, extension + " File");
        }
        
        SetItem(itemIndex, 3, formatDateTime(entry.modifiedTime));
        
    } else { // GRID_MODE: same m_entries, show icon + label
        long itemIndex = InsertItem(index, wxString::FromUTF8(entry.name.c_str()), iconIndex);
        (void)itemIndex;
    }
}

std::string FileListView::formatFileSize(std::uint64_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double displaySize = static_cast<double>(size);
    
    while (displaySize >= 1024.0 && unitIndex < 4) {
        displaySize /= 1024.0;
        unitIndex++;
    }
    
    std::ostringstream oss;
    if (unitIndex == 0) {
        oss << static_cast<uint64_t>(displaySize) << " " << units[unitIndex];
    } else {
        oss << std::fixed << std::setprecision(1) << displaySize << " " << units[unitIndex];
    }
    
    return oss.str();
}

std::string FileListView::formatDateTime(std::uint64_t timestamp) {
    if (timestamp == 0) return "";
    
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::localtime(&time);
    
    if (!tm) return "";
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

void FileListView::OnItemSelected(wxListEvent& event) {
    long index = event.GetIndex();
    if (index >= 0 && index < static_cast<long>(m_entries.size())) {
        VolumeFile file(m_location.volume, joinVolumePath(m_location.path, m_entries[index].name));
        notifyFileSelected(file);
    }
    
    std::vector<VolumeFile> files;
    std::vector<std::string> selected = getSelectedFiles();
    for (const auto& name : selected) {
        VolumeFile file(m_location.volume, joinVolumePath(m_location.path, name));
        files.push_back(file);
    }
    notifySelectionChanged(files);
    event.Skip();
}

void FileListView::OnItemActivated(wxListEvent& event) {
    long index = event.GetIndex();
    if (index >= 0 && index < static_cast<long>(m_entries.size())) {
        VolumeFile file(m_location.volume, joinVolumePath(m_location.path, m_entries[index].name));
        notifyFileActivated(file);
    }
    event.Skip();
}

void FileListView::OnItemRightClick(wxListEvent& event) {
    long index = event.GetIndex();
    if (index < 0 || index >= static_cast<long>(m_entries.size())) return;
    
    const DirEntry& entry = m_entries[index];
    
    // Create context menu
    wxMenu contextMenu;
    
    if (entry.isDirectory()) {
        contextMenu.Append(wxID_OPEN, "Open");
        contextMenu.AppendSeparator();
    } else {
        contextMenu.Append(wxID_OPEN, "Open");
        contextMenu.Append(wxID_EDIT, "Edit");
        contextMenu.AppendSeparator();
    }
    
    contextMenu.Append(wxID_COPY, "Copy");
    contextMenu.Append(wxID_CUT, "Cut");
    contextMenu.Append(wxID_DELETE, "Delete");
    contextMenu.AppendSeparator();
    contextMenu.Append(wxID_PROPERTIES, "Properties");
    
    // Show context menu
    PopupMenu(&contextMenu);
    
    event.Skip();
}

void FileListView::OnColumnClick(wxListEvent& event) {
    int column = event.GetColumn();
    
    if (column == m_sortColumn) {
        m_sortAscending = !m_sortAscending;
    } else {
        m_sortAscending = true;
        m_sortColumn = column;
    }
    
    switch (column) {
        case 0: sortByName(m_sortAscending); break;
        case 1: sortBySize(m_sortAscending); break;
        case 2: sortByType(m_sortAscending); break;
        case 3: sortByDate(m_sortAscending); break;
        default: break;
    }
    updateColumnHeaders();
    event.Skip();
}

void FileListView::OnKeyDown(wxKeyEvent& event) {
    int keyCode = event.GetKeyCode();
    
    switch (keyCode) {
        case WXK_DELETE:
            if (!getSelectedFiles().empty()) {
                std::vector<VolumeFile> files;
                for (const auto& item : getSelectedFiles()) {
                    VolumeFile file(m_location.volume, joinVolumePath(m_location.path, item));
                    files.push_back(file);
                }
                notifySelectionChanged(files);
            }
            event.Skip();
            break;
            
        case WXK_F2:
            if (getSelectedFiles().size() == 1) {
                // TODO: Implement rename functionality
            }
            event.Skip();
            break;
            
        case WXK_RETURN:
            if (!getSelectedFiles().empty()) {
                auto selected = getSelectedFiles();
                for (const auto& item : selected) {
                    VolumeFile file(m_location.volume, joinVolumePath(m_location.path, item));
                    notifyFileActivated(file);
                }
            }
            event.Skip();
            break;
            
        default:
            event.Skip();
            break;
    }
}

void FileListView::OnLeftDown(wxMouseEvent& event) {
    m_rectSelecting = true;
    m_rectStartPos = event.GetPosition();
    int flags = 0;
    m_rectStartItem = HitTest(m_rectStartPos, flags);
    event.Skip();
}

void FileListView::OnLeftUp(wxMouseEvent& event) {
    if (m_rectSelecting) {
        m_rectSelecting = false;
        std::vector<VolumeFile> files;
        for (const auto& name : getSelectedFiles()) {
            VolumeFile file(m_location.volume, joinVolumePath(m_location.path, name));
            files.push_back(file);
        }
        notifySelectionChanged(files);
    }
    event.Skip();
}

void FileListView::OnMotion(wxMouseEvent& event) {
    if (m_rectSelecting && event.LeftIsDown()) {
        int flags = 0;
        int currentItem = HitTest(event.GetPosition(), flags);
        if (currentItem >= 0 && m_rectStartItem >= 0) {
            int from = std::min(m_rectStartItem, currentItem);
            int to = std::max(m_rectStartItem, currentItem);
            for (int i = 0; i < GetItemCount(); ++i) {
                SetItemState(i, (i >= from && i <= to) ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED);
            }
        }
    }
    event.Skip();
}

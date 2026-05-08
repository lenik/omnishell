#include "FileListView.hpp"

#include "../../core/App.hpp"
#include "../../wx/WxChecked.hpp"

#include <bas/ui/arch/ImageSet.hpp>
#include <bas/util/FileTypeDetector.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/wx/artprovs.hpp>
#include <bas/wx/images.hpp>

#include <wx/artprov.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

enum IconIndex {
    ICON_INDEX_UNKNOWN = -1,
    ICON_INDEX_FOLDER = 0,
    ICON_INDEX_FILE = 1,
    ICON_INDEX_IMAGE = 2,
};

enum ColumnIndex {
    COL_NAME = 0,
    COL_SIZE = 1,
    COL_TYPE = 2,
    COL_MODIFIED = 3,
};

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
                    EVT_KEY_DOWN(FileListView::OnKeyDown) EVT_LEFT_DOWN(FileListView::OnLeftDown)
                        EVT_LEFT_UP(FileListView::OnLeftUp) EVT_MOTION(FileListView::OnMotion)
                            wxEND_EVENT_TABLE()

                                static wxBitmap IconBitmapForList(const wxArtID& id, int size) {
    ImageSet image(id);

    static const wxColour kPink = wxColour(255, 192, 203);
    BitmapMode mode = BitmapMode::DEFAULT.backcolor(kPink);
    return image.toBitmap1(size, size, wxART_LIST, mode);
}

static wxBitmap SolidFallbackBitmap(int w, int h,   //
                                    wxColour back,  // = wxColour(220, 220, 220),
                                    wxColour border // = wxColour(180, 180, 180)
) {
    wxBitmap bmp(w, h, 24);
    if (!bmp.IsOk())
        return bmp;
    wxMemoryDC dc(bmp);
    dc.SetBackground(wxBrush(back));
    dc.Clear();
    dc.SetPen(wxPen(border));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(0, 0, w, h);
    return bmp;
}

static std::string thumbKey(const std::string& path, int size) {
    return path + "|" + std::to_string(size);
}

static std::string variantKey(const std::string& path, int size, size_t fadeSteps, int variantId) {
    return path + "|" + std::to_string(size) + "|" + std::to_string(fadeSteps) + "|" +
           std::to_string(variantId);
}

static wxImage adjustBrightnessAndMaybeGreyscale(const wxImage& src, float brightness,
                                                 bool greyscale) {
    wxImage out = src;
    if (!out.IsOk())
        return out;
    if (greyscale)
        out = out.ConvertToGreyscale();
    unsigned char* d = out.GetData();
    if (!d)
        return out;
    brightness = std::clamp(brightness, 0.0f, 1.0f);
    const size_t px = static_cast<size_t>(out.GetWidth()) * static_cast<size_t>(out.GetHeight());
    for (size_t i = 0; i < px; ++i) {
        const size_t o = i * 3;
        d[o + 0] = static_cast<unsigned char>(static_cast<float>(d[o + 0]) * brightness);
        d[o + 1] = static_cast<unsigned char>(static_cast<float>(d[o + 1]) * brightness);
        d[o + 2] = static_cast<unsigned char>(static_cast<float>(d[o + 2]) * brightness);
    }
    return out;
}

static wxImage withBorder(const wxImage& src, const wxColour& borderColor, int thickness) {
    wxImage out = src;
    if (!out.IsOk())
        return out;
    unsigned char* d = out.GetData();
    if (!d)
        return out;

    const int w = out.GetWidth();
    const int h = out.GetHeight();
    if (w <= 0 || h <= 0)
        return out;

    thickness = std::max(1, thickness);
    thickness = std::min(thickness, std::min(w, h) / 2);

    auto setPx = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= w || y >= h)
            return;
        const size_t o =
            static_cast<size_t>(y) * static_cast<size_t>(w) * 3u + static_cast<size_t>(x) * 3u;
        d[o + 0] = borderColor.Red();
        d[o + 1] = borderColor.Green();
        d[o + 2] = borderColor.Blue();
    };

    for (int t = 0; t < thickness; ++t) {
        for (int x = t; x < w - t; ++x) {
            setPx(x, t);
            setPx(x, h - 1 - t);
        }
        for (int y = t; y < h - t; ++y) {
            setPx(t, y);
            setPx(w - 1 - t, y);
        }
    }
    return out;
}

FileListView::FileListView(wxWindow* parent, const Location& location)
    : wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                 wxLC_REPORT | wxLC_SORT_ASCENDING),
      m_location(location), m_viewMode(LIST_MODE), m_imageListSmall(nullptr),
      m_imageListLarge(nullptr), m_sortColumn(0), m_sortAscending(true), m_rectSelecting(false),
      m_rectStartItem(-1) {

    SetName(wxT("file_list"));
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

    auto install = [this](const ImageSet& set) {
        auto small = set.toBitmap1(ICON_SIZE_SMALL, ICON_SIZE_SMALL, wxART_LIST);
        auto large = set.toBitmap1(ICON_SIZE_LARGE, ICON_SIZE_LARGE, wxART_LIST);
        m_imageListSmall->Add(small);
        m_imageListLarge->Add(large);
    };

    const os::IconTheme* theme = os::app.getIconTheme();
    install(theme->icon("explorer", "list.folder"));
    install(theme->icon("explorer", "list.file"));
    install(theme->icon("explorer", "list.image"));
}

int FileListView::getIconIndex(const DirEntry& entry, size_t listIndex) {
    const int size = (m_viewMode == LIST_MODE) ? ICON_SIZE_SMALL : ICON_SIZE_LARGE;
    wxImageList* imgList = (m_viewMode == LIST_MODE) ? m_imageListSmall : m_imageListLarge;
    if (entry.isDirectory())
        return ICON_INDEX_FOLDER;
    using FT = FileTypeDetector::FileType;
    FT ft = FileTypeDetector::detectFileType(entry.name);

    if (ft == FT::IMAGE && entry.size > 0 && entry.size <= m_thumbnailMaxBytes &&
        m_location.volume) {
        const std::string path = joinVolumePath(m_location.path, entry.name);

        const size_t total = m_entries.size();
        const bool hasRecentWindow = m_recencyFadeEnabled && m_recencyRecentCount > 0 && total > 0;

        const size_t recentCount = hasRecentWindow ? std::min(m_recencyRecentCount, total) : 0;
        const bool isRecent = hasRecentWindow ? (listIndex + 1 > total - recentCount) : false;

        int variantId = 0; // 0 = recent (with border), >=1 = faded steps
        float brightness = 1.0f;
        bool greyscale = true;
        bool needBorder = false;

        if (hasRecentWindow) {
            if (isRecent) {
                variantId = 0;
                brightness = 1.0f;
                greyscale = false;
                needBorder = m_recencyBorderEnabled;
            } else {
                const size_t nonRecentCount = total - recentCount;
                const size_t idxInNonRecent = std::min(listIndex, nonRecentCount - 1);
                // Distance from the recent window boundary (in number of items).
                const size_t distFromRecent =
                    (nonRecentCount == 0) ? 0 : (nonRecentCount - 1 - idxInNonRecent);
                size_t steps = std::max<size_t>(2, m_recencyFadeSteps);
                const size_t olderLevels =
                    (steps > 2) ? (steps - 2) : 0; // 0..olderLevels -> older items
                const size_t maxFadeDist = std::max<size_t>(1, m_recencyRecentCount * 4);
                const size_t clampedDist = std::min(distFromRecent, maxFadeDist);
                const size_t olderLevel0 =
                    (olderLevels == 0) ? 0 : (clampedDist * olderLevels) / maxFadeDist;
                variantId = 1 + static_cast<int>(olderLevel0);

                // Map variantId to brightness [~0.2..1.0]
                if (steps > 2) {
                    const float t =
                        static_cast<float>(variantId - 1) / static_cast<float>(steps - 2);
                    brightness = 1.0f - t * 0.8f;
                } else {
                    brightness = 0.3f; // only two steps: keep it dimmer than recent
                }
                greyscale = true;
                needBorder = false;
            }
        } else {
            // Default non-recency path: create normal thumbnails (variantId=0, no border).
            variantId = 1;
            brightness = 1.0f;
            greyscale = false;
            needBorder = false;
        }

        const std::string baseKey = thumbKey(path, size);
        const std::string vKey = variantKey(path, size, m_recencyFadeSteps, variantId);

        if (auto it = m_thumbnailVariantCache.find(vKey); it != m_thumbnailVariantCache.end())
            return it->second;

        try {
            wxImage base;
            if (auto it = m_thumbnailBaseCache.find(baseKey); it != m_thumbnailBaseCache.end()) {
                base = it->second;
            } else {
                VolumeFile vf(m_location.volume, path);
                std::vector<uint8_t> data = vf.readFile();
                if (data.empty())
                    return ICON_INDEX_IMAGE;
                wxMemoryInputStream stream(data.data(), data.size());
                if (!base.LoadFile(stream, wxBITMAP_TYPE_ANY) || !base.IsOk())
                    return ICON_INDEX_IMAGE;
                base = base.Scale(size, size, wxIMAGE_QUALITY_BILINEAR);
                if (!base.IsOk())
                    return ICON_INDEX_IMAGE;
                m_thumbnailBaseCache[baseKey] = base;
            }

            wxImage variant = adjustBrightnessAndMaybeGreyscale(base, brightness, greyscale);
            if (needBorder) {
                // Use a warm highlight border.
                wxColour borderColor(255, 230, 160);
                const int thickness = std::max(1, size / 12);
                variant = withBorder(variant, borderColor, thickness);
            }

            wxBitmap bmp(variant);
            if (!bmp.IsOk())
                return ICON_INDEX_IMAGE;
            int idx = imgList->Add(bmp);
            m_thumbnailVariantCache[vKey] = idx;
            return idx;
        } catch (...) {
        }
    }
    /* Do not load/decode files here: synchronous read+decode per row slowed Explorer and could
       produce wxBitmaps without valid GTK pixbufs (ImageList::Draw assert). */
    if (ft == FT::IMAGE)
        return ICON_INDEX_IMAGE;
    return ICON_INDEX_FILE;
}

void FileListView::updateColumnHeaders() {
    if (m_viewMode != LIST_MODE || GetColumnCount() < 4)
        return;
    const char* base[] = {"Name", "Size", "Type", "Modified"};
    for (int c = 0; c < 4; ++c) {
        wxString label = base[c];
        if (c == m_sortColumn)
            label += m_sortAscending ? " \u25B2" : " \u25BC";
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
    m_thumbnailBaseCache.clear();
    m_thumbnailVariantCache.clear();
    DeleteAllItems();
    populateList();
}

void FileListView::refresh() { populateList(); }

void FileListView::setEntryFilter(std::function<bool(const DirEntry&)> filter) {
    m_entryFilter = std::move(filter);
}

void FileListView::clearEntryFilter() { m_entryFilter = nullptr; }

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

void FileListView::enableRecencyFade(size_t recentCount, size_t fadeSteps, bool border) {
    m_recencyFadeEnabled = recentCount > 0;
    m_recencyRecentCount = recentCount;
    m_recencyFadeSteps = std::max<size_t>(2, fadeSteps);
    m_recencyBorderEnabled = border;
    // Variant cache depends on fadeSteps, so clear it.
    m_thumbnailVariantCache.clear();
    Refresh();
}

void FileListView::disableRecencyFade() {
    m_recencyFadeEnabled = false;
    m_recencyRecentCount = 0;
    m_thumbnailVariantCache.clear();
    Refresh();
}

void FileListView::setThumbnailMaxBytes(size_t maxBytes) {
    m_thumbnailMaxBytes = maxBytes;
    // Existing thumbnails are still valid; no need to clear.
    Refresh();
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
                  return ascending ? (a.epochNano < b.epochNano) : (a.epochNano > b.epochNano);
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
        m_entries.reserve(fileStatuses->children.size());
        for (const auto& [name, child] : fileStatuses->children) {
            if (child) {
                m_entries.push_back(*child);
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
        wxMessageBox("Error reading directory: " + std::string(e.what()), "Error",
                     wxOK | wxICON_ERROR);
    }
}

void FileListView::updateEntry(int index, const DirEntry& entry) {
    int iconIndex = getIconIndex(entry, index);
    if (m_viewMode == LIST_MODE) {
        long itemIndex = InsertItem(index, wxString::FromUTF8(entry.name.c_str()), iconIndex);

        if (entry.isDirectory()) {
            SetItem(itemIndex, COL_SIZE, "<DIR>");
        } else {
            SetItem(itemIndex, COL_SIZE, formatFileSize(entry.size));
        }

        if (entry.isDirectory()) {
            SetItem(itemIndex, COL_TYPE, "Folder");
        } else {
            size_t dotPos = entry.name.find_last_of('.');
            std::string extension =
                (dotPos != std::string::npos) ? entry.name.substr(dotPos + 1) : "File";
            SetItem(itemIndex, COL_TYPE, extension + " File");
        }

        SetItem(itemIndex, COL_MODIFIED,
                formatDateTime(static_cast<std::uint64_t>(entry.epochSeconds())));

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
    if (timestamp == 0)
        return "";

    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::localtime(&time);

    if (!tm)
        return "";

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

void FileListView::OnItemSelected(wxListEvent& event) {
    os::wxInvokeChecked(this, &event, [&] {
        long index = event.GetIndex();
        if (index >= 0 && index < static_cast<long>(m_entries.size())) {
            VolumeFile file(m_location.volume,
                            joinVolumePath(m_location.path, m_entries[index].name));
            notifyFileSelected(file);
        }

        std::vector<VolumeFile> files;
        std::vector<std::string> selected = getSelectedFiles();
        for (const auto& name : selected) {
            VolumeFile file(m_location.volume, joinVolumePath(m_location.path, name));
            files.push_back(file);
        }
        notifySelectionChanged(files);
    });
    event.Skip();
}

void FileListView::OnItemActivated(wxListEvent& event) {
    os::wxInvokeChecked(this, &event, [&] {
        long index = event.GetIndex();
        if (index >= 0 && index < static_cast<long>(m_entries.size())) {
            VolumeFile file(m_location.volume,
                            joinVolumePath(m_location.path, m_entries[index].name));
            notifyFileActivated(file);
        }
    });
    event.Skip();
}

void FileListView::OnItemRightClick(wxListEvent& event) {
    os::wxInvokeChecked(this, &event, [&] {
        long index = event.GetIndex();
        if (index < 0 || index >= static_cast<long>(m_entries.size()))
            return;

        const DirEntry& entry = m_entries[index];

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

        PopupMenu(&contextMenu);
    });
    event.Skip();
}

void FileListView::OnColumnClick(wxListEvent& event) {
    os::wxInvokeChecked(this, &event, [&] {
        int column = event.GetColumn();

        if (column == m_sortColumn) {
            m_sortAscending = !m_sortAscending;
        } else {
            m_sortAscending = true;
            m_sortColumn = column;
        }

        switch (column) {
        case COL_NAME:
            sortByName(m_sortAscending);
            break;
        case COL_SIZE:
            sortBySize(m_sortAscending);
            break;
        case COL_TYPE:
            sortByType(m_sortAscending);
            break;
        case COL_MODIFIED:
            sortByDate(m_sortAscending);
            break;
        default:
            break;
        }
        updateColumnHeaders();
    });
    event.Skip();
}

void FileListView::OnKeyDown(wxKeyEvent& event) {
    os::wxInvokeChecked(this, &event, [&] {
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
            break;

        case WXK_F2:
            if (getSelectedFiles().size() == 1) {
                // TODO: Implement rename functionality
            }
            break;

        case WXK_RETURN:
            if (!getSelectedFiles().empty()) {
                auto selected = getSelectedFiles();
                for (const auto& item : selected) {
                    VolumeFile file(m_location.volume, joinVolumePath(m_location.path, item));
                    notifyFileActivated(file);
                }
            }
            break;

        default:
            break;
        }
    });
    event.Skip();
}

void FileListView::OnLeftDown(wxMouseEvent& event) {
    m_rectSelecting = true;
    m_rectStartPos = event.GetPosition();
    int flags = 0;
    m_rectStartItem = HitTest(m_rectStartPos, flags);
    event.Skip();
}

void FileListView::OnLeftUp(wxMouseEvent& event) {
    os::wxInvokeChecked(this, &event, [&] {
        if (m_rectSelecting) {
            m_rectSelecting = false;
            std::vector<VolumeFile> files;
            for (const auto& name : getSelectedFiles()) {
                VolumeFile file(m_location.volume, joinVolumePath(m_location.path, name));
                files.push_back(file);
            }
            notifySelectionChanged(files);
        }
    });
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
                SetItemState(i, (i >= from && i <= to) ? wxLIST_STATE_SELECTED : 0,
                             wxLIST_STATE_SELECTED);
            }
        }
    }
    event.Skip();
}

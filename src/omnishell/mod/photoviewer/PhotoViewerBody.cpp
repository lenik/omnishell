#include "PhotoViewerBody.hpp"

#include <bas/volume/VolumeFile.hpp>

#include <wx/dcbuffer.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/utils.h>

#include <algorithm>
#include <cctype>

namespace os {

namespace {

enum {
    ID_PHOTO_PREV = 140001,
    ID_PHOTO_NEXT = 140002,
    ID_PHOTO_DELETE = 140003,
    ID_GROUP_FILE = 140010,
};

static std::string lowerAscii(std::string s) {
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static std::string dirOf(const std::string& path) {
    const size_t pos = path.find_last_of('/');
    if (pos == std::string::npos)
        return "/";
    if (pos == 0)
        return "/";
    return path.substr(0, pos);
}

static std::string baseNameOf(const std::string& path) {
    const size_t pos = path.find_last_of('/');
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

static bool hasImageExt(const std::string& name) {
    const std::string n = lowerAscii(name);
    const size_t dot = n.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= n.size())
        return false;
    const std::string ext = n.substr(dot + 1);
    return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "webp" || ext == "bmp" ||
           ext == "gif" || ext == "tif" || ext == "tiff";
}

} // namespace

PhotoViewerBody::PhotoViewerBody() {
    // File group (optional menu/tool integration)
    group(ID_GROUP_FILE, "", "file", 1000)
        .label("&File")
        .description("File operations")
        .install();

    int seq = 0;
    action(ID_PHOTO_PREV, "file", "prev", seq++, "&Previous", "Previous image")
        .shortcuts({"Left", "Up", "PageUp"})
        .performFn([this](PerformContext* ctx) { onPrev(ctx); })
        .install();

    action(ID_PHOTO_NEXT, "file", "next", seq++, "&Next", "Next image")
        .shortcuts({"Right", "Down", "PageDown"})
        .performFn([this](PerformContext* ctx) { onNext(ctx); })
        .install();

    action(ID_PHOTO_DELETE, "file", "delete", seq++, "&Delete", "Delete current image")
        .shortcut("Del")
        .performFn([this](PerformContext* ctx) { onDelete(ctx); })
        .install();
}

void PhotoViewerBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    m_root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    m_root->SetBackgroundColour(*wxBLACK);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    m_view = new wxStaticBitmap(m_root, wxID_ANY, wxNullBitmap, wxDefaultPosition,
                                wxDefaultSize, wxBORDER_NONE);
    m_view->SetBackgroundColour(*wxBLACK);
    sizer->Add(m_view, 1, wxEXPAND | wxALL, 0);
    m_root->SetSizer(sizer);

    m_root->Bind(wxEVT_SIZE, [this](wxSizeEvent&) { updateShownBitmap(); });
    updateShownBitmap();
}

void PhotoViewerBody::loadVolumeFile(const VolumeFile& file) {
    // VolumeFile is already an abstraction over the underlying volume.
    m_file = file;
    m_dirPath = dirOf(file.getPath());
    refreshDirImageList();
    try {
        const auto data = file.readFile();
        if (data.empty())
            return;

        wxMemoryInputStream ms(data.data(), data.size());
        wxImage img;
        if (!img.LoadFile(ms, wxBITMAP_TYPE_ANY) || !img.IsOk())
            return;

        m_original = img;
        updateShownBitmap();

        if (wxWindow* tlw = m_root ? wxGetTopLevelParent(m_root) : nullptr) {
            const std::string name = baseNameOf(file.getPath());
            tlw->SetLabel("Photo Viewer - " + name);
        }
    } catch (...) {
        // Keep the viewer empty on failure; no hard crash.
    }
}

void PhotoViewerBody::refreshDirImageList() {
    m_dirImages.clear();
    m_dirIndex = -1;

    if (!m_file)
        return;
    Volume* vol = m_file->getVolume();
    if (!vol)
        return;

    // List same directory entries
    VolumeFile dirFile(vol, m_dirPath.empty() ? "/" : m_dirPath);
    auto dir = dirFile.readDir(false);
    try {
        dir = dirFile.readDir(false);
    } catch (...) {
        return;
    }

    struct Item {
        std::string path;
        std::chrono::system_clock::time_point mtime;
        std::string name;
    };
    std::vector<Item> items;
    items.reserve(dir->children.size());

    for (const auto& [name, child] : dir->children) {
        if (!child->isRegularFile())
            continue;
        if (!hasImageExt(name))
            continue;
        std::string full =
            (m_dirPath.empty() || m_dirPath == "/") ? ("/" + name) : (m_dirPath + "/" + name);
        auto mtime = child->modifiedTime();
        items.push_back({full, mtime, name});
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        if (a.mtime != b.mtime)
            return a.mtime < b.mtime;
        return a.name < b.name;
    });

    m_dirImages.reserve(items.size());
    for (const auto& it : items)
        m_dirImages.push_back(it.path);

    // Find current index
    const std::string curPath = m_file->getPath();
    for (size_t i = 0; i < m_dirImages.size(); ++i) {
        if (m_dirImages[i] == curPath) {
            m_dirIndex = static_cast<int>(i);
            break;
        }
    }
}

void PhotoViewerBody::showDirIndex(int index) {
    if (!m_file)
        return;
    if (index < 0 || index >= static_cast<int>(m_dirImages.size()))
        return;

    Volume* vol = m_file->getVolume();
    if (!vol)
        return;

    m_dirIndex = index;
    VolumeFile vf(vol, m_dirImages[static_cast<size_t>(index)]);
    loadVolumeFile(vf);
}

void PhotoViewerBody::onPrev(PerformContext*) {
    if (m_dirImages.empty() || m_dirIndex < 0)
        refreshDirImageList();
    if (m_dirImages.empty() || m_dirIndex < 0)
        return;
    showDirIndex(std::max(0, m_dirIndex - 1));
}

void PhotoViewerBody::onNext(PerformContext*) {
    if (m_dirImages.empty() || m_dirIndex < 0)
        refreshDirImageList();
    if (m_dirImages.empty() || m_dirIndex < 0)
        return;
    showDirIndex(std::min(static_cast<int>(m_dirImages.size()) - 1, m_dirIndex + 1));
}

void PhotoViewerBody::onDelete(PerformContext*) {
    if (!m_file)
        return;

    const std::string curPath = m_file->getPath();
    const std::string name = baseNameOf(curPath);

    const int ret = wxMessageBox("Delete this image?\n\n" + name, "Photo Viewer",
                                 wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
    if (ret != wxYES)
        return;

    // Decide where to go after deletion
    refreshDirImageList();
    int nextIndex = m_dirIndex;
    if (nextIndex < 0)
        nextIndex = 0;

    // Try to delete
    try {
        const bool ok = m_file->removeFile();
        if (!ok) {
            wxMessageBox("Delete failed.", "Photo Viewer", wxOK | wxICON_ERROR);
            return;
        }
    } catch (...) {
        wxMessageBox("Delete failed.", "Photo Viewer", wxOK | wxICON_ERROR);
        return;
    }

    // Refresh list and move
    refreshDirImageList();
    if (m_dirImages.empty()) {
        // Nothing left; close window
        if (wxWindow* tlw = m_root ? wxGetTopLevelParent(m_root) : nullptr)
            tlw->Close();
        return;
    }
    if (nextIndex >= static_cast<int>(m_dirImages.size()))
        nextIndex = static_cast<int>(m_dirImages.size()) - 1;
    showDirIndex(nextIndex);
}

void PhotoViewerBody::updateShownBitmap() {
    if (!m_view)
        return;
    if (!m_original.IsOk())
        return;

    wxSize box = m_view->GetClientSize();
    if (box.x <= 2 || box.y <= 2)
        return;

    const int sw = m_original.GetWidth();
    const int sh = m_original.GetHeight();
    if (sw <= 0 || sh <= 0)
        return;

    // Preserve aspect ratio and fit into the view box.
    const double sx = static_cast<double>(box.x) / static_cast<double>(sw);
    const double sy = static_cast<double>(box.y) / static_cast<double>(sh);
    const double s = std::min(sx, sy);
    const int dw = std::max(1, static_cast<int>(sw * s));
    const int dh = std::max(1, static_cast<int>(sh * s));

    wxImage scaled = m_original.Scale(dw, dh, wxIMAGE_QUALITY_NORMAL);
    if (!scaled.IsOk())
        return;

    wxBitmap canvas(box.x, box.y);
    if (!canvas.IsOk())
        return;

    wxMemoryDC dc(canvas);
    dc.SetBackground(wxBrush(*wxBLACK));
    dc.Clear();

    const int ox = (box.x - dw) / 2;
    const int oy = (box.y - dh) / 2;
    dc.DrawBitmap(wxBitmap(scaled), ox, oy, true);
    dc.SelectObject(wxNullBitmap);

    m_view->SetBitmap(canvas);
}

} // namespace os


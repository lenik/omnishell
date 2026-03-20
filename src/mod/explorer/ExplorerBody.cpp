#include "ExplorerBody.hpp"

#include "../../core/registry/FileAssociations.hpp"
#include "../../shell/Shell.hpp"
#include "../browser/BrowserApp.hpp"
#include "../notepad/NotepadApp.hpp"
#include "../paint/PaintApp.hpp"
#include "ui/ThemeStyles.hpp"

using namespace ThemeStyles;

#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>
#include <wx/sizer.h>

#include <vector>

namespace os {

namespace {
enum {
    ID_GROUP_NAV = uiFrame::ID_APP_HIGHEST + 50,
    ID_UP,
    ID_REFRESH,
};

static std::string joinPath(const std::string& base, const std::string& name) {
    if (base.empty() || base == "/")
        return "/" + name;
    if (base.back() == '/')
        return base + name;
    return base + "/" + name;
}

static std::string parentPath(const std::string& p) {
    if (p.empty() || p == "/")
        return "/";
    std::string s = p;
    while (s.size() > 1 && s.back() == '/')
        s.pop_back();
    size_t slash = s.rfind('/');
    if (slash == std::string::npos || slash == 0)
        return "/";
    return s.substr(0, slash);
}
} // namespace

ExplorerBody::ExplorerBody(VolumeManager* vm, Volume* volume, std::string dir)
    : m_vm(vm), m_volume(volume), m_dir(std::move(dir)) {

    group(ID_GROUP_NAV, "navigate", "explorer", 1000, "&Navigate", "Explorer navigation").install();
    int seq = 0;
    action(ID_UP, "navigate/explorer", "up", seq++)
        .label("&Up")
        .description("Go to parent folder")
        .icon(wxART_GO_BACK, hi_normal, "arrow-uturn-left.svg")
        .performFn([this](PerformContext* ctx) { onUp(ctx); })
        .install();
    action(ID_REFRESH, "navigate/explorer", "refresh", seq++)
        .label("&Refresh")
        .description("Refresh directory")
        .icon(wxART_REDO, hi_normal, "arrow-path.svg")
        .performFn([this](PerformContext* ctx) { onRefresh(ctx); })
        .install();
}

void ExplorerBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    wxPanel* root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

    m_pathLabel = new wxStaticText(root, wxID_ANY, "");
    rootSizer->Add(m_pathLabel, 0, wxEXPAND | wxALL, 5);

    wxSplitterWindow* split = new wxSplitterWindow(root, wxID_ANY);
    split->SetSashGravity(0.25);
    split->SetMinimumPaneSize(160);

    wxPanel* left = new wxPanel(split, wxID_ANY);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    m_tree =
        new wxTreeCtrl(left, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT | wxTR_DEFAULT_STYLE);
    leftSizer->Add(m_tree, 1, wxEXPAND | wxALL, 6);
    left->SetSizer(leftSizer);

    wxPanel* right = new wxPanel(split, wxID_ANY);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    m_list = new wxListCtrl(right, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL);
    m_list->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 420);
    m_list->InsertColumn(1, "Type", wxLIST_FORMAT_LEFT, 100);
    m_list->InsertColumn(2, "Size", wxLIST_FORMAT_RIGHT, 100);
    rightSizer->Add(m_list, 1, wxEXPAND | wxALL, 6);
    right->SetSizer(rightSizer);

    split->SplitVertically(left, right);
    rootSizer->Add(split, 1, wxEXPAND);

    // Tree image list (volume icons) - streamline-vectors
    m_treeImages = new wxImageList(16, 16);
    auto addTreeIcon = [this](std::string_view dir, std::string_view tail) {
        ImageSet set(Path(dir, tail));
        wxBitmap bmp = set.toBitmap1(16, 16);
        return bmp.IsOk() ? m_treeImages->Add(bmp) : -1;
    };
    // Use flat computer-device icons for volumes
    m_volIcon[(int)VolumeType::HARDDISK] =
        addTreeIcon(slv_core_flat, "computer-devices/hard-drive-1.svg");
    m_volIcon[(int)VolumeType::NETWORK] =
        addTreeIcon(slv_core_flat, "computer-devices/network.svg");
    m_volIcon[(int)VolumeType::MEMORY] =
        addTreeIcon(slv_core_flat, "computer-devices/database-server-1.svg");
    m_volIcon[(int)VolumeType::CDROM] =
        addTreeIcon(slv_freehand_duotone, "computers-devices-electronics/cd-rom-disc-1.svg");
    m_volIcon[(int)VolumeType::ARCHIVE] =
        addTreeIcon(slv_core_pop, "interface-essential/archive-box.svg");
    // Fallbacks
    m_volIcon[(int)VolumeType::SYSTEM] = m_volIcon[(int)VolumeType::MEMORY];
    m_volIcon[(int)VolumeType::FLOPPY] = m_volIcon[(int)VolumeType::HARDDISK];
    m_volIcon[(int)VolumeType::OTHER] =
        addTreeIcon(slv_core_pop, "interface-essential/folder-add.svg");
    m_tree->AssignImageList(m_treeImages);

    // List image list (file icons) - streamline-vectors
    m_listImages = new wxImageList(16, 16);
    auto addListIcon = [this](std::string_view dir, std::string_view tail) {
        ImageSet set(Path(dir, tail));
        wxBitmap bmp = set.toBitmap1(16, 16);
        return bmp.IsOk() ? m_listImages->Add(bmp) : -1;
    };
    m_iconFolder = addListIcon(slv_core_pop, "interface-essential/folder-add.svg");
    m_iconFileGeneric = addListIcon(slv_core_pop, "interface-essential/file-add-alternate.svg");
    m_iconFileImage = addListIcon(slv_core_pop, "images-photography/edit-image-photo.svg");
    m_iconFileText = addListIcon(slv_core_pop, "interface-essential/text-square.svg");
    m_iconFileAudio = addListIcon(slv_core_pop, "entertainment/music-note-1.svg");
    m_iconFileVideo = addListIcon(slv_core_pop, "entertainment/camera-video.svg");
    m_iconFilePdf = addListIcon(slv_core_pop, "interface-essential/convert-pdf-2.svg");
    m_iconFileCode = addListIcon(slv_core_pop, "programming/code-monitor-1.svg");
    m_iconFileArchive = addListIcon(slv_core_pop, "interface-essential/archive-box.svg");
    m_iconFileSheet = addListIcon(slv_core_pop, "interface-essential/text-flow-rows.svg");
    m_list->SetImageList(m_listImages, wxIMAGE_LIST_SMALL);

    m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent& e) {
        if (!m_list)
            return;
        long item = e.GetIndex();
        wxString name = m_list->GetItemText(item, 0);
        wxString type = m_list->GetItemText(item, 1);
        if (type == "Directory")
            openChild(name.ToStdString(), true);
        else
            openFile(name.ToStdString());
    });

    m_tree->Bind(wxEVT_TREE_ITEM_EXPANDING, [this](wxTreeEvent& e) {
        loadTreeChildren(e.GetItem());
        e.Skip();
    });
    m_tree->Bind(wxEVT_TREE_SEL_CHANGED, [this](wxTreeEvent& e) {
        wxTreeItemId id = e.GetItem();
        auto* d = id.IsOk() ? dynamic_cast<TreeData*>(m_tree->GetItemData(id)) : nullptr;
        if (d && d->volume) {
            m_volume = d->volume;
            setDir(d->path.empty() ? "/" : d->path);
        }
        e.Skip();
    });

    root->SetSizer(rootSizer);
    refreshTreeVolumes();
    if (m_volume) {
        selectTreePath(m_volume, m_dir);
    }
    refresh();
}

wxEvtHandler* ExplorerBody::getEventHandler() {
    return m_list ? m_list->GetEventHandler() : (m_tree ? m_tree->GetEventHandler() : nullptr);
}

void ExplorerBody::setDir(std::string dir) {
    m_dir = std::move(dir);
    refresh();
    selectTreePath(m_volume, m_dir);
}

void ExplorerBody::refresh() {
    if (!m_volume || !m_list || !m_pathLabel)
        return;
    try {
        auto entries = m_volume->readDir(m_dir, false);
        m_pathLabel->SetLabel(
            wxString::Format("Volume: %s   Path: %s", m_volume->getLabel().c_str(), m_dir.c_str()));
        m_list->DeleteAllItems();
        long index = 0;
        for (const auto& e : entries) {
            wxString name = e->name;
            wxString type = e->isDirectory() ? "Directory" : "File";
            wxString size;
            if (!e->isDirectory()) {
                size.Printf("%llu", static_cast<unsigned long long>(e->size));
            }
            int img = m_iconFileGeneric;
            if (e->isDirectory()) {
                img = m_iconFolder;
            } else {
                std::string n = name.ToStdString();
                auto dot = n.rfind('.');
                if (dot != std::string::npos) {
                    std::string ext = n.substr(dot + 1);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    img = listIconForExtension(ext);
                }
            }

            long row = m_list->InsertItem(index, name, img);
            m_list->SetItem(row, 1, type);
            if (!e->isDirectory()) {
                m_list->SetItem(row, 2, size);
            }
            ++index;
        }
        if (m_frame) {
            m_frame->SetTitle(wxString::Format("Explorer - %s", m_dir.c_str()));
        }
    } catch (const std::exception& ex) {
        wxMessageBox(wxString("Cannot open directory: ") + ex.what(), "Explorer",
                     wxOK | wxICON_ERROR);
    }
}

void ExplorerBody::refreshTreeVolumes() {
    if (!m_tree || !m_vm)
        return;
    m_tree->DeleteAllItems();
    wxTreeItemId root = m_tree->AddRoot("");

    for (const auto& up : m_vm->all()) {
        Volume* v = up.get();
        if (!v)
            continue;
        wxString label = v->getLabel().empty() ? v->getId() : v->getLabel();
        VolumeType t = v->getType();
        int img = m_volIcon[static_cast<int>(t)];
        wxTreeItemId vid = m_tree->AppendItem(root, label, img, img, new TreeData(v, "/"));
        // lazy placeholder
        m_tree->AppendItem(vid, "(loading...)");
    }
    m_tree->Expand(root);
}

void ExplorerBody::loadTreeChildren(const wxTreeItemId& id) {
    if (!m_tree || !id.IsOk())
        return;
    auto* d = dynamic_cast<TreeData*>(m_tree->GetItemData(id));
    if (!d || !d->volume)
        return;
    if (d->loaded)
        return;
    d->loaded = true;
    m_tree->DeleteChildren(id);

    try {
        auto entries = d->volume->readDir(d->path.empty() ? "/" : d->path, false);
        for (const auto& e : entries) {
            if (!e->isDirectory())
                continue;
            std::string childPath = joinPath(d->path.empty() ? "/" : d->path, e->name);
            wxTreeItemId cid =
                m_tree->AppendItem(id, e->name, -1, -1, new TreeData(d->volume, childPath));
            m_tree->AppendItem(cid, "(loading...)");
        }
    } catch (...) {
    }
}

void ExplorerBody::selectTreePath(Volume* vol, const std::string& path) {
    if (!m_tree || !vol)
        return;
    wxTreeItemId root = m_tree->GetRootItem();
    if (!root.IsOk())
        return;

    // Find volume node
    wxTreeItemIdValue cookie;
    wxTreeItemId vid = m_tree->GetFirstChild(root, cookie);
    while (vid.IsOk()) {
        auto* d = dynamic_cast<TreeData*>(m_tree->GetItemData(vid));
        if (d && d->volume == vol)
            break;
        vid = m_tree->GetNextChild(root, cookie);
    }
    if (!vid.IsOk())
        return;

    // Ensure expanded and children loaded
    loadTreeChildren(vid);
    m_tree->Expand(vid);

    std::string p = path.empty() ? "/" : path;
    if (p == "/") {
        m_tree->SelectItem(vid);
        return;
    }

    // Walk segments
    std::string curPath = "/";
    wxTreeItemId curId = vid;
    size_t start = 0;
    while (start < p.size()) {
        while (start < p.size() && p[start] == '/')
            start++;
        if (start >= p.size())
            break;
        size_t slash = p.find('/', start);
        std::string seg =
            (slash == std::string::npos) ? p.substr(start) : p.substr(start, slash - start);
        curPath = joinPath(curPath, seg);

        loadTreeChildren(curId);
        m_tree->Expand(curId);
        wxTreeItemIdValue ck2;
        wxTreeItemId child = m_tree->GetFirstChild(curId, ck2);
        wxTreeItemId found;
        while (child.IsOk()) {
            auto* cd = dynamic_cast<TreeData*>(m_tree->GetItemData(child));
            if (cd && cd->volume == vol && cd->path == curPath) {
                found = child;
                break;
            }
            child = m_tree->GetNextChild(curId, ck2);
        }
        if (!found.IsOk())
            break;
        curId = found;
        start = (slash == std::string::npos) ? p.size() : slash + 1;
    }
    if (curId.IsOk()) {
        m_tree->SelectItem(curId);
        m_tree->EnsureVisible(curId);
    }
}

void ExplorerBody::openChild(const std::string& name, bool isDir) {
    if (!isDir)
        return;
    const std::string next = joinPath(m_dir, name);
    setDir(next);
}

namespace {
bool extIn(const std::string& ext, const std::vector<std::string>& list) {
    for (const auto& e : list)
        if (ext == e)
            return true;
    return false;
}
} // namespace

int ExplorerBody::listIconForExtension(const std::string& ext) const {
    if (ext.empty())
        return m_iconFileGeneric;
    static const std::vector<std::string> image = {"png",  "jpg", "jpeg", "jfif", "bmp", "gif",
                                                   "webp", "ico", "svg",  "tiff", "tif", "heic",
                                                   "avif", "raw", "cr2",  "nef",  "arw"};
    static const std::vector<std::string> audio = {"mp3", "wav",  "flac", "ogg", "m4a", "aac",
                                                   "wma", "opus", "aiff", "aif", "ape", "alac",
                                                   "mid", "midi", "au",   "snd"};
    static const std::vector<std::string> video = {"mp4", "mkv", "avi",  "webm", "mov", "wmv",
                                                   "flv", "m4v", "mpeg", "mpg",  "3gp", "3g2",
                                                   "ogv", "vob", "ts",   "m2ts", "divx"};
    static const std::vector<std::string> code = {
        "c",    "cpp",  "cc",  "cxx", "h",   "hh",   "hpp", "hxx",   "py",     "pyw",   "js",
        "ts",   "jsx",  "tsx", "mjs", "cjs", "rs",   "go",  "java",  "kt",     "kts",   "swift",
        "vb",   "cs",   "fs",  "rb",  "pl",  "pm",   "php", "scala", "r",      "lua",   "sh",
        "bash", "zsh",  "bat", "cmd", "ps1", "html", "htm", "xhtml", "css",    "vue",   "sass",
        "scss", "less", "sql", "rkt", "clj", "cljs", "ex",  "exs",   "erl",    "hrl",   "hs",
        "lhs",  "ml",   "mli", "fsi", "nim", "zig",  "v",   "dart",  "groovy", "gradle"};
    static const std::vector<std::string> text = {
        "txt", "md",   "markdown", "log",  "ini",     "cfg",      "conf",  "json",
        "xml", "yaml", "yml",      "toml", "rst",     "tex",      "latex", "bib",
        "doc", "docx", "odt",      "rtf",  "texinfo", "asciidoc", "adoc"};
    static const std::vector<std::string> archive = {
        "zip",  "7z",  "rar",  "tar",  "gz",  "bz2", "xz",  "z",   "lz",
        "lzma", "tgz", "tbz2", "tzst", "zst", "lz4", "br",  "jar", "war",
        "ear",  "cab", "deb",  "rpm",  "dmg", "iso", "cpio"};
    static const std::vector<std::string> sheet = {"xls",     "xlsx", "ods", "csv", "tsv",
                                                   "numbers", "dif",  "slk", "wb2", "wks"};
    if (extIn(ext, image) && m_iconFileImage >= 0)
        return m_iconFileImage;
    if (extIn(ext, audio) && m_iconFileAudio >= 0)
        return m_iconFileAudio;
    if (extIn(ext, video) && m_iconFileVideo >= 0)
        return m_iconFileVideo;
    if (ext == "pdf" && m_iconFilePdf >= 0)
        return m_iconFilePdf;
    if (extIn(ext, code) && m_iconFileCode >= 0)
        return m_iconFileCode;
    if (extIn(ext, text) && m_iconFileText >= 0)
        return m_iconFileText;
    if (extIn(ext, archive) && m_iconFileArchive >= 0)
        return m_iconFileArchive;
    if (extIn(ext, sheet) && m_iconFileSheet >= 0)
        return m_iconFileSheet;
    if (extIn(ext, text))
        return m_iconFileText >= 0 ? m_iconFileText : m_iconFileGeneric;
    if (extIn(ext, code))
        return m_iconFileCode >= 0 ? m_iconFileCode : m_iconFileGeneric;
    return m_iconFileGeneric;
}

static bool looksLikeTextFile(const std::string& name) {
    auto dot = name.rfind('.');
    if (dot == std::string::npos)
        return false;
    std::string ext = name.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    static const char* kExts[] = {"txt", "md",  "log", "ini", "cfg", "conf", "json", "xml", "yaml",
                                  "yml", "csv", "tsv", "cpp", "hpp", "h",    "c",    "cc",  "hh",
                                  "py",  "sh",  "js",  "ts",  "css", "html", "htm",  "rs",  "go"};
    for (auto* e : kExts) {
        if (ext == e)
            return true;
    }
    return false;
}

void ExplorerBody::openFile(const std::string& name) {
    if (!m_volume || !m_vm)
        return;
    const std::string path = joinPath(m_dir, name);
    try {
        VolumeFile vf(m_volume, path);
        auto data = vf.readFile();
        if (!data.empty()) {
            wxMemoryInputStream ms(data.data(), data.size());
            wxImage img(ms, wxBITMAP_TYPE_ANY);
            if (img.IsOk()) {
                ProcessPtr p = PaintApp::openImage(m_vm, vf);
                if (auto shell = ShellApp::getInstance()) {
                    if (p && shell->getTaskbar())
                        shell->getTaskbar()->addProcess(p);
                }
                return;
            }
        }
        auto dot = name.rfind('.');
        if (dot != std::string::npos) {
            std::string ext = name.substr(dot + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (auto modUri = getFirstOpenWithModuleForExtension(ext)) {
                ProcessPtr p;
                if (*modUri == "omnishell.Browser")
                    p = BrowserApp::open(m_vm, vf);
                else if (*modUri == "omnishell.Notepad")
                    p = NotepadApp::open(m_vm, vf);
                if (p) {
                    if (auto shell = ShellApp::getInstance()) {
                        if (shell->getTaskbar())
                            shell->getTaskbar()->addProcess(p);
                    }
                    return;
                }
            }
        }

        if (looksLikeTextFile(name)) {
            ProcessPtr p = NotepadApp::open(m_vm, vf);
            if (auto shell = ShellApp::getInstance()) {
                if (p && shell->getTaskbar())
                    shell->getTaskbar()->addProcess(p);
            }
            return;
        }
        wxMessageBox("No associated app for this file type.", "Explorer",
                     wxOK | wxICON_INFORMATION);
    } catch (...) {
    }
}

void ExplorerBody::onUp(PerformContext*) { setDir(parentPath(m_dir)); }

void ExplorerBody::onRefresh(PerformContext*) { refresh(); }

} // namespace os

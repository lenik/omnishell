#include "ExplorerCore.hpp"

#include "../../shell/Shell.hpp"
#include "../notepad/NotepadApp.hpp"
#include "../paint/PaintApp.hpp"

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

ExplorerCore::ExplorerCore(VolumeManager* vm, Volume* volume, std::string dir)
    : vm_(vm)
    , volume_(volume)
    , dir_(std::move(dir)) {
    const std::string icons = "heroicons/normal";

    group(ID_GROUP_NAV, "navigate", "explorer", 1000, "&Navigate", "Explorer navigation").install();
    int seq = 0;
    action(ID_UP, "navigate/explorer", "up", seq++, "&Up", "Go to parent folder")
        .icon(wxART_GO_BACK, icons, "arrow-uturn-left.svg")
        .performFn([this](PerformContext* ctx) { onUp(ctx); })
        .install();
    action(ID_REFRESH, "navigate/explorer", "refresh", seq++, "&Refresh", "Refresh directory")
        .icon(wxART_REDO, icons, "arrow-path.svg")
        .performFn([this](PerformContext* ctx) { onRefresh(ctx); })
        .install();
}

void ExplorerCore::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    frame_ = frame;

    wxPanel* root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

    pathLabel_ = new wxStaticText(root, wxID_ANY, "");
    rootSizer->Add(pathLabel_, 0, wxEXPAND | wxALL, 5);

    wxSplitterWindow* split = new wxSplitterWindow(root, wxID_ANY);
    split->SetSashGravity(0.25);
    split->SetMinimumPaneSize(160);

    wxPanel* left = new wxPanel(split, wxID_ANY);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    tree_ = new wxTreeCtrl(left, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT |
                               wxTR_HIDE_ROOT | wxTR_DEFAULT_STYLE);
    leftSizer->Add(tree_, 1, wxEXPAND | wxALL, 6);
    left->SetSizer(leftSizer);

    wxPanel* right = new wxPanel(split, wxID_ANY);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    list_ = new wxListCtrl(right, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxLC_REPORT | wxLC_SINGLE_SEL);
    list_->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 420);
    list_->InsertColumn(1, "Type", wxLIST_FORMAT_LEFT, 100);
    list_->InsertColumn(2, "Size", wxLIST_FORMAT_RIGHT, 100);
    rightSizer->Add(list_, 1, wxEXPAND | wxALL, 6);
    right->SetSizer(rightSizer);

    split->SplitVertically(left, right);
    rootSizer->Add(split, 1, wxEXPAND);

    // Tree image list (volume icons) - streamline-vectors
    treeImages_ = new wxImageList(16, 16);
    auto addTreeIcon = [this](const char* relDir, const char* file) {
        ImageSet set(Path(std::string("streamline-vectors/") + relDir, file));
        wxBitmap bmp = set.toBitmap1(16, 16);
        return bmp.IsOk() ? treeImages_->Add(bmp) : -1;
    };
    // Use flat computer-device icons for volumes
    volIcon_[(int)VolumeType::HARDDISK] =
        addTreeIcon("core/flat/computer-devices", "hard-drive-1.svg");
    volIcon_[(int)VolumeType::NETWORK] =
        addTreeIcon("core/flat/computer-devices", "network.svg");
    volIcon_[(int)VolumeType::MEMORY] =
        addTreeIcon("core/flat/computer-devices", "database-server-1.svg");
    volIcon_[(int)VolumeType::CDROM] =
        addTreeIcon("freehand/duotone/computers-devices-electronics", "cd-rom-disc-1.svg");
    volIcon_[(int)VolumeType::ARCHIVE] =
        addTreeIcon("core/pop/interface-essential", "archive-box.svg");
    // Fallbacks
    volIcon_[(int)VolumeType::SYSTEM] = volIcon_[(int)VolumeType::MEMORY];
    volIcon_[(int)VolumeType::FLOPPY] = volIcon_[(int)VolumeType::HARDDISK];
    volIcon_[(int)VolumeType::OTHER] =
        addTreeIcon("block/interface-essential/content", "folder.svg");
    tree_->AssignImageList(treeImages_);

    // List image list (file icons) - streamline-vectors
    listImages_ = new wxImageList(16, 16);
    auto addListIcon = [this](const char* relDir, const char* file) {
        ImageSet set(Path(std::string("streamline-vectors/") + relDir, file));
        wxBitmap bmp = set.toBitmap1(16, 16);
        return bmp.IsOk() ? listImages_->Add(bmp) : -1;
    };
    iconFolder_ =
        addListIcon("block/interface-essential/content", "folder.svg");
    iconFileGeneric_ =
        addListIcon("block/interface-essential/content", "file.svg");
    iconFileImage_ =
        addListIcon("block/interface-essential/content", "image.svg");
    // Use text-field as generic text/office file icon
    iconFileText_ =
        addListIcon("block/interface-essential/text-formatting", "text-field.svg");
    iconFileAudio_ =
        addListIcon("ultimate/bold/files-folders", "audio-file-mp-3.svg");
    iconFileVideo_ =
        addListIcon("block/interface-essential/content", "video.svg");
    iconFilePdf_ =
        addListIcon("core/pop/interface-essential", "convert-pdf-2.svg");
    iconFileCode_ =
        addListIcon("core/pop/programming", "code-monitor-1.svg");
    iconFileArchive_ =
        addListIcon("flex/pop/interface-essential", "zip-folder.svg");
    iconFileSheet_ =
        addListIcon("freehand/duotone/work-office-companies", "office-file-sheet.svg");
    list_->SetImageList(listImages_, wxIMAGE_LIST_SMALL);

    list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent& e) {
        if (!list_)
            return;
        long item = e.GetIndex();
        wxString name = list_->GetItemText(item, 0);
        wxString type = list_->GetItemText(item, 1);
        if (type == "Directory")
            openChild(name.ToStdString(), true);
        else
            openFile(name.ToStdString());
    });

    tree_->Bind(wxEVT_TREE_ITEM_EXPANDING, [this](wxTreeEvent& e) {
        loadTreeChildren(e.GetItem());
        e.Skip();
    });
    tree_->Bind(wxEVT_TREE_SEL_CHANGED, [this](wxTreeEvent& e) {
        wxTreeItemId id = e.GetItem();
        auto* d = id.IsOk() ? dynamic_cast<TreeData*>(tree_->GetItemData(id)) : nullptr;
        if (d && d->volume) {
            volume_ = d->volume;
            setDir(d->path.empty() ? "/" : d->path);
        }
        e.Skip();
    });

    root->SetSizer(rootSizer);
    refreshTreeVolumes();
    if (volume_) {
        selectTreePath(volume_, dir_);
    }
    refresh();
}

wxEvtHandler* ExplorerCore::getEventHandler() {
    return list_ ? list_->GetEventHandler() : (tree_ ? tree_->GetEventHandler() : nullptr);
}

void ExplorerCore::setDir(std::string dir) {
    dir_ = std::move(dir);
    refresh();
    selectTreePath(volume_, dir_);
}

void ExplorerCore::refresh() {
    if (!volume_ || !list_ || !pathLabel_)
        return;
    try {
        auto entries = volume_->readDir(dir_, false);
        pathLabel_->SetLabel(wxString::Format("Volume: %s   Path: %s",
                                              volume_->getLabel().c_str(), dir_.c_str()));
        list_->DeleteAllItems();
        long index = 0;
        for (const auto& e : entries) {
            wxString name = e->name;
            wxString type = e->isDirectory() ? "Directory" : "File";
            wxString size;
            if (!e->isDirectory()) {
                size.Printf("%llu", static_cast<unsigned long long>(e->size));
            }
            int img = iconFileGeneric_;
            if (e->isDirectory()) {
                img = iconFolder_;
            } else {
                std::string n = name.ToStdString();
                auto dot = n.rfind('.');
                if (dot != std::string::npos) {
                    std::string ext = n.substr(dot + 1);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    img = listIconForExtension(ext);
                }
            }

            long row = list_->InsertItem(index, name, img);
            list_->SetItem(row, 1, type);
            if (!e->isDirectory()) {
                list_->SetItem(row, 2, size);
            }
            ++index;
        }
        if (frame_) {
            frame_->SetTitle(wxString::Format("Explorer - %s", dir_.c_str()));
        }
    } catch (const std::exception& ex) {
        wxMessageBox(wxString("Cannot open directory: ") + ex.what(), "Explorer", wxOK | wxICON_ERROR);
    }
}

void ExplorerCore::refreshTreeVolumes() {
    if (!tree_ || !vm_)
        return;
    tree_->DeleteAllItems();
    wxTreeItemId root = tree_->AddRoot("");

    for (const auto& up : vm_->all()) {
        Volume* v = up.get();
        if (!v)
            continue;
        wxString label = v->getLabel().empty() ? v->getId() : v->getLabel();
        VolumeType t = v->getType();
        int img = volIcon_[static_cast<int>(t)];
        wxTreeItemId vid = tree_->AppendItem(root, label, img, img, new TreeData(v, "/"));
        // lazy placeholder
        tree_->AppendItem(vid, "(loading...)");
    }
    tree_->Expand(root);
}

void ExplorerCore::loadTreeChildren(const wxTreeItemId& id) {
    if (!tree_ || !id.IsOk())
        return;
    auto* d = dynamic_cast<TreeData*>(tree_->GetItemData(id));
    if (!d || !d->volume)
        return;
    if (d->loaded)
        return;
    d->loaded = true;
    tree_->DeleteChildren(id);

    try {
        auto entries = d->volume->readDir(d->path.empty() ? "/" : d->path, false);
        for (const auto& e : entries) {
            if (!e->isDirectory())
                continue;
            std::string childPath = joinPath(d->path.empty() ? "/" : d->path, e->name);
            wxTreeItemId cid = tree_->AppendItem(id, e->name, -1, -1, new TreeData(d->volume, childPath));
            tree_->AppendItem(cid, "(loading...)");
        }
    } catch (...) {
    }
}

void ExplorerCore::selectTreePath(Volume* vol, const std::string& path) {
    if (!tree_ || !vol)
        return;
    wxTreeItemId root = tree_->GetRootItem();
    if (!root.IsOk())
        return;

    // Find volume node
    wxTreeItemIdValue cookie;
    wxTreeItemId vid = tree_->GetFirstChild(root, cookie);
    while (vid.IsOk()) {
        auto* d = dynamic_cast<TreeData*>(tree_->GetItemData(vid));
        if (d && d->volume == vol)
            break;
        vid = tree_->GetNextChild(root, cookie);
    }
    if (!vid.IsOk())
        return;

    // Ensure expanded and children loaded
    loadTreeChildren(vid);
    tree_->Expand(vid);

    std::string p = path.empty() ? "/" : path;
    if (p == "/") {
        tree_->SelectItem(vid);
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
        std::string seg = (slash == std::string::npos) ? p.substr(start) : p.substr(start, slash - start);
        curPath = joinPath(curPath, seg);

        loadTreeChildren(curId);
        tree_->Expand(curId);
        wxTreeItemIdValue ck2;
        wxTreeItemId child = tree_->GetFirstChild(curId, ck2);
        wxTreeItemId found;
        while (child.IsOk()) {
            auto* cd = dynamic_cast<TreeData*>(tree_->GetItemData(child));
            if (cd && cd->volume == vol && cd->path == curPath) {
                found = child;
                break;
            }
            child = tree_->GetNextChild(curId, ck2);
        }
        if (!found.IsOk())
            break;
        curId = found;
        start = (slash == std::string::npos) ? p.size() : slash + 1;
    }
    if (curId.IsOk()) {
        tree_->SelectItem(curId);
        tree_->EnsureVisible(curId);
    }
}

void ExplorerCore::openChild(const std::string& name, bool isDir) {
    if (!isDir)
        return;
    const std::string next = joinPath(dir_, name);
    setDir(next);
}

namespace {
bool extIn(const std::string& ext, const std::vector<std::string>& list) {
    for (const auto& e : list)
        if (ext == e) return true;
    return false;
}
} // namespace

int ExplorerCore::listIconForExtension(const std::string& ext) const {
    if (ext.empty()) return iconFileGeneric_;
    static const std::vector<std::string> image = {
        "png","jpg","jpeg","jfif","bmp","gif","webp","ico","svg","tiff","tif","heic","avif","raw","cr2","nef","arw"
    };
    static const std::vector<std::string> audio = {
        "mp3","wav","flac","ogg","m4a","aac","wma","opus","aiff","aif","ape","alac","mid","midi","au","snd"
    };
    static const std::vector<std::string> video = {
        "mp4","mkv","avi","webm","mov","wmv","flv","m4v","mpeg","mpg","3gp","3g2","ogv","vob","ts","m2ts","divx"
    };
    static const std::vector<std::string> code = {
        "c","cpp","cc","cxx","h","hh","hpp","hxx","py","pyw","js","ts","jsx","tsx","mjs","cjs","rs","go","java","kt","kts","swift","vb","cs","fs","rb","pl","pm","php","scala","r","lua","sh","bash","zsh","bat","cmd","ps1","html","htm","xhtml","css","vue","sass","scss","less","sql","rkt","clj","cljs","ex","exs","erl","hrl","hs","lhs","ml","mli","fsi","nim","zig","v","dart","groovy","gradle"
    };
    static const std::vector<std::string> text = {
        "txt","md","markdown","log","ini","cfg","conf","json","xml","yaml","yml","toml","rst","tex","latex","bib","doc","docx","odt","rtf","texinfo","asciidoc","adoc"
    };
    static const std::vector<std::string> archive = {
        "zip","7z","rar","tar","gz","bz2","xz","z","lz","lzma","tgz","tbz2","tzst","zst","lz4","br","jar","war","ear","cab","deb","rpm","dmg","iso","cpio"
    };
    static const std::vector<std::string> sheet = {
        "xls","xlsx","ods","csv","tsv","numbers","dif","slk","wb2","wks"
    };
    if (extIn(ext, image) && iconFileImage_ >= 0) return iconFileImage_;
    if (extIn(ext, audio) && iconFileAudio_ >= 0) return iconFileAudio_;
    if (extIn(ext, video) && iconFileVideo_ >= 0) return iconFileVideo_;
    if (ext == "pdf" && iconFilePdf_ >= 0) return iconFilePdf_;
    if (extIn(ext, code) && iconFileCode_ >= 0) return iconFileCode_;
    if (extIn(ext, text) && iconFileText_ >= 0) return iconFileText_;
    if (extIn(ext, archive) && iconFileArchive_ >= 0) return iconFileArchive_;
    if (extIn(ext, sheet) && iconFileSheet_ >= 0) return iconFileSheet_;
    if (extIn(ext, text)) return iconFileText_ >= 0 ? iconFileText_ : iconFileGeneric_;
    if (extIn(ext, code)) return iconFileCode_ >= 0 ? iconFileCode_ : iconFileGeneric_;
    return iconFileGeneric_;
}

static bool looksLikeTextFile(const std::string& name) {
    auto dot = name.rfind('.');
    if (dot == std::string::npos)
        return false;
    std::string ext = name.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    static const char* kExts[] = {"txt","md","log","ini","cfg","conf","json","xml","yaml","yml","csv","tsv",
                                  "cpp","hpp","h","c","cc","hh","py","sh","js","ts","css","html","htm","rs","go"};
    for (auto* e : kExts) {
        if (ext == e)
            return true;
    }
    return false;
}

void ExplorerCore::openFile(const std::string& name) {
    if (!volume_ || !vm_)
        return;
    const std::string path = joinPath(dir_, name);
    try {
        VolumeFile vf(volume_, path);
        auto data = vf.readFile();
        if (!data.empty()) {
            wxMemoryInputStream ms(data.data(), data.size());
            wxImage img(ms, wxBITMAP_TYPE_ANY);
            if (img.IsOk()) {
                ProcessPtr p = PaintApp::openImage(vm_, vf);
                if (auto shell = ShellApp::getInstance()) {
                    if (p && shell->getTaskbar())
                        shell->getTaskbar()->addProcess(p);
                }
                return;
            }
        }
        if (looksLikeTextFile(name)) {
            ProcessPtr p = NotepadApp::open(vm_, vf);
            if (auto shell = ShellApp::getInstance()) {
                if (p && shell->getTaskbar())
                    shell->getTaskbar()->addProcess(p);
            }
            return;
        }
        wxMessageBox("No associated app for this file type.", "Explorer", wxOK | wxICON_INFORMATION);
    } catch (...) {
    }
}

void ExplorerCore::onUp(PerformContext*) {
    setDir(parentPath(dir_));
}

void ExplorerCore::onRefresh(PerformContext*) {
    refresh();
}

} // namespace os


#include "RegistryCore.hpp"

#include "../../core/RegistryDb.hpp"

#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/textdlg.h>
#include <wx/treectrl.h>

#include <map>
#include <set>

namespace os {

namespace {
static std::vector<std::string> splitKey(const std::string& s) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start < s.size()) {
        size_t dot = s.find('.', start);
        if (dot == std::string::npos) {
            out.push_back(s.substr(start));
            break;
        }
        out.push_back(s.substr(start, dot - start));
        start = dot + 1;
    }
    return out;
}

static std::string joinPath(const std::string& base, const std::string& seg) {
    if (base.empty())
        return seg;
    return base + "." + seg;
}

enum {
    ID_RELOAD = uiFrame::ID_APP_HIGHEST + 1,
    ID_GROUP_REG = uiFrame::ID_APP_HIGHEST + 50,
};

} // namespace

RegistryCore::RegistryCore() {
    std::string dir = "heroicons/normal";

    group(ID_GROUP_REG, "file", "registry", 1000, "&Registry", "Registry actions").install();
    action(ID_RELOAD, "file/registry", "reload", 0, "&Reload", "Reload registry")
        .icon(wxART_REDO, dir, "arrow-path.svg")
        .performFn([this](PerformContext* ctx) { onReload(ctx); })
        .install();
}

void RegistryCore::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    frame_ = frame;

    RegistryDb::getInstance().load();

    wxSplitterWindow* split = new wxSplitterWindow(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    split->SetSashGravity(0.30);
    split->SetMinimumPaneSize(160);

    wxPanel* left = new wxPanel(split, wxID_ANY);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    tree_ = new wxTreeCtrl(left, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_DEFAULT_STYLE);
    leftSizer->Add(tree_, 1, wxEXPAND | wxALL, 6);
    left->SetSizer(leftSizer);

    wxPanel* right = new wxPanel(split, wxID_ANY);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    list_ = new wxListCtrl(right, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    list_->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 220);
    list_->InsertColumn(1, "Value", wxLIST_FORMAT_LEFT, 420);
    rightSizer->Add(list_, 1, wxEXPAND | wxALL, 6);
    right->SetSizer(rightSizer);

    split->SplitVertically(left, right);

    tree_->Bind(wxEVT_TREE_SEL_CHANGED, [this](wxTreeEvent& e) {
        wxTreeItemId id = e.GetItem();
        RegistryTreePathData* d =
            id.IsOk() ? dynamic_cast<RegistryTreePathData*>(tree_->GetItemData(id)) : nullptr;
        selectedPath_ = d ? d->path : "";
        populateProperties(selectedPath_);
        e.Skip();
    });
    list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent&) { editSelectedProperty(); });

    buildTree();
    selectedPath_.clear();
    populateProperties(selectedPath_);
}

wxEvtHandler* RegistryCore::getEventHandler() {
    return tree_ ? tree_->GetEventHandler() : nullptr;
}

void RegistryCore::onReload(PerformContext*) {
    RegistryDb::getInstance().load();
    buildTree();
    populateProperties(selectedPath_);
}

void RegistryCore::buildTree() {
    if (!tree_)
        return;
    tree_->DeleteAllItems();
    wxTreeItemId root = tree_->AddRoot("Registry", -1, -1, new RegistryTreePathData(""));

    const auto& data = RegistryDb::getInstance().data();
    std::set<std::string> folderPaths;
    folderPaths.insert("");
    for (const auto& kv : data) {
        auto parts = splitKey(kv.first);
        std::string path;
        for (size_t i = 0; i + 1 < parts.size(); ++i) {
            path = joinPath(path, parts[i]);
            folderPaths.insert(path);
        }
    }

    std::map<std::string, wxTreeItemId> nodeByPath;
    nodeByPath[""] = root;

    for (const auto& folderPath : folderPaths) {
        if (folderPath.empty())
            continue;
        auto parts = splitKey(folderPath);
        std::string path;
        wxTreeItemId parent = root;
        for (const auto& part : parts) {
            std::string next = joinPath(path, part);
            if (!folderPaths.count(next))
                break;
            auto it = nodeByPath.find(next);
            if (it == nodeByPath.end()) {
                wxTreeItemId nid =
                    tree_->AppendItem(parent, part, -1, -1, new RegistryTreePathData(next));
                nodeByPath[next] = nid;
                parent = nid;
            } else {
                parent = it->second;
            }
            path = next;
        }
    }

    tree_->Expand(root);
}

void RegistryCore::populateProperties(const std::string& nodePath) {
    if (!list_)
        return;
    list_->DeleteAllItems();

    const auto& data = RegistryDb::getInstance().data();
    const std::string prefix = nodePath.empty() ? "" : (nodePath + ".");

    struct Entry {
        std::string name;
        bool hasValue{false};
        std::string value;
    };
    std::map<std::string, Entry> entries;

    for (const auto& kv : data) {
        const std::string& key = kv.first;
        if (!prefix.empty()) {
            if (key.rfind(prefix, 0) != 0)
                continue;
        }
        std::string rest = prefix.empty() ? key : key.substr(prefix.size());
        if (rest.empty())
            continue;
        size_t dot = rest.find('.');
        std::string child = dot == std::string::npos ? rest : rest.substr(0, dot);

        auto& e = entries[child];
        e.name = child;
        if (dot == std::string::npos) {
            e.hasValue = true;
            e.value = kv.second;
        }
    }

    long idx = 0;
    for (const auto& [name, e] : entries) {
        list_->InsertItem(idx, name);
        list_->SetItem(idx, 1, e.hasValue ? e.value : "(folder)");
        ++idx;
    }
}

void RegistryCore::editSelectedProperty() {
    if (!list_)
        return;
    long item = -1;
    item = list_->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item == -1)
        return;

    wxString name = list_->GetItemText(item, 0);
    wxString val = list_->GetItemText(item, 1);
    if (val == "(folder)") {
        const std::string next = selectedPath_.empty() ? name.ToStdString()
                                                       : (selectedPath_ + "." + name.ToStdString());
        wxTreeItemId root = tree_->GetRootItem();
        std::vector<std::string> parts = splitKey(next);
        wxTreeItemId cur = root;
        for (const auto& p : parts) {
            wxTreeItemIdValue cookie;
            wxTreeItemId child = tree_->GetFirstChild(cur, cookie);
            bool found = false;
            while (child.IsOk()) {
                if (tree_->GetItemText(child).ToStdString() == p) {
                    cur = child;
                    found = true;
                    break;
                }
                child = tree_->GetNextChild(cur, cookie);
            }
            if (!found)
                break;
        }
        if (cur.IsOk())
            tree_->SelectItem(cur);
        return;
    }

    const std::string fullKey =
        selectedPath_.empty() ? name.ToStdString() : (selectedPath_ + "." + name.ToStdString());

    wxTextEntryDialog dlg(frame_, "Value:", "Edit Registry Value", val);
    if (dlg.ShowModal() != wxID_OK)
        return;
    wxString newVal = dlg.GetValue();

    RegistryDb::getInstance().set(fullKey, newVal.ToStdString());
    RegistryDb::getInstance().save();
    populateProperties(selectedPath_);
}

} // namespace os


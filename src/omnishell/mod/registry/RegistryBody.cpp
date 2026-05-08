#include "RegistryBody.hpp"

#include "../../core/RegistryDb.hpp"
#include "../../core/registry/RegistryKeyUtil.hpp"
#include "../../core/registry/RegistryPath.hpp"

#include "../../core/App.hpp"
#include "../../ui/ThemeStyles.hpp"

#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/textdlg.h>
#include <wx/treectrl.h>

#include <algorithm>
#include <functional>
#include <map>
#include <set>

namespace os {

namespace {

size_t minPos(size_t a, size_t b) {
    if (a == std::string::npos)
        return b;
    if (b == std::string::npos)
        return a;
    return std::min(a, b);
}

std::string firstSeg(const std::string& s) {
    size_t slash = s.find('/');
    size_t dot = s.find('.');
    size_t end = minPos(slash, dot);
    if (end == std::string::npos)
        return s;
    return s.substr(0, end);
}

void addFolderPrefixes(std::set<std::string>& fp, const std::string& key) {
    using reg::splitKeyByChar;
    auto slash = splitKeyByChar(key, '/');
    if (slash.empty())
        return;
    std::string acc;
    for (size_t i = 0; i + 1 < slash.size(); ++i) {
        if (i)
            acc += '/';
        acc += slash[i];
        fp.insert(acc);
    }
    std::string baseSlash;
    if (slash.size() >= 2) {
        for (size_t i = 0; i + 1 < slash.size(); ++i) {
            if (i)
                baseSlash += '/';
            baseSlash += slash[i];
        }
    }
    auto dots = splitKeyByChar(slash.back(), '.');
    for (size_t j = 0; j + 1 < dots.size(); ++j) {
        std::string p = baseSlash;
        if (!p.empty())
            p += '/';
        for (size_t k = 0; k <= j; ++k) {
            if (k)
                p += '.';
            p += dots[k];
        }
        fp.insert(p);
    }
}

std::string parentDir(const std::set<std::string>& s, const std::string& path) {
    for (size_t i = path.size(); i-- > 0;) {
        if (path[i] == '/' || path[i] == '.') {
            std::string p = path.substr(0, i);
            if (s.count(p))
                return p;
        }
    }
    return s.count("") ? std::string{""} : std::string{};
}

std::string childLabel(const std::string& parent, const std::string& path) {
    if (parent.empty())
        return firstSeg(path);
    if (path.size() > parent.size() && path.compare(0, parent.size(), parent) == 0 &&
        path.size() > parent.size() &&
        (path[parent.size()] == '/' || path[parent.size()] == '.')) {
        return firstSeg(path.substr(parent.size() + 1));
    }
    return firstSeg(path);
}

enum {
    ID_RELOAD = uiFrame::ID_APP_HIGHEST + 1,
    ID_GROUP_REG = uiFrame::ID_APP_HIGHEST + 50,
};

} // namespace

RegistryBody::RegistryBody() {
    auto theme = os::app.getIconTheme();

    group(ID_GROUP_REG, "file", "registry", 1000, "&Registry", "Registry actions").install();
    action(ID_RELOAD, "file/registry", "reload", 0, "&Reload", "Reload registry")
        .icon(theme->icon("registry", "reload"))
        .performFn([this](PerformContext* ctx) { onReload(ctx); })
        .install();
}

wxWindow* RegistryBody::createFragmentView(CreateViewContext* ctx) {
    RegistryDb::getInstance().load();

    m_frame = ctx->findParentFrame();
    
    wxWindow* parent = ctx->getParent();

    wxSplitterWindow* split = new wxSplitterWindow(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    split->SetSashGravity(0.30);
    split->SetMinimumPaneSize(160);

    wxPanel* left = new wxPanel(split, wxID_ANY);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    m_tree = new wxTreeCtrl(left, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_DEFAULT_STYLE);
    leftSizer->Add(m_tree, 1, wxEXPAND | wxALL, 6);
    left->SetSizer(leftSizer);

    wxPanel* right = new wxPanel(split, wxID_ANY);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    m_list = new wxListCtrl(right, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    m_list->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 220);
    m_list->InsertColumn(1, "Value", wxLIST_FORMAT_LEFT, 420);
    rightSizer->Add(m_list, 1, wxEXPAND | wxALL, 6);
    right->SetSizer(rightSizer);

    split->SplitVertically(left, right);

    m_tree->Bind(wxEVT_TREE_SEL_CHANGED, [this](wxTreeEvent& e) {
        wxTreeItemId id = e.GetItem();
        RegistryTreePathData* d =
            id.IsOk() ? dynamic_cast<RegistryTreePathData*>(m_tree->GetItemData(id)) : nullptr;
        m_selectedPath = d ? d->path : "";
        populateProperties(m_selectedPath);
        e.Skip();
    });
    m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent&) { editSelectedProperty(); });

    buildTree();
    m_selectedPath.clear();
    populateProperties(m_selectedPath);

    return split;
}

void RegistryBody::onReload(PerformContext*) {
    RegistryDb::getInstance().load();
    buildTree();
    populateProperties(m_selectedPath);
}

void RegistryBody::buildTree() {
    if (!m_tree)
        return;
    m_tree->DeleteAllItems();
    wxTreeItemId root = m_tree->AddRoot("Registry", -1, -1, new RegistryTreePathData(""));

    const auto data = RegistryDb::getInstance().snapshotStrings();
    std::set<std::string> folderPaths;
    folderPaths.insert("");
    for (const auto& kv : data)
        addFolderPrefixes(folderPaths, kv.first);

    std::map<std::string, wxTreeItemId> nodeByPath;
    nodeByPath[""] = root;

    for (const auto& folderPath : folderPaths) {
        if (folderPath.empty())
            continue;
        std::string pdir = parentDir(folderPaths, folderPath);
        auto pit = nodeByPath.find(pdir);
        if (pit == nodeByPath.end())
            continue;
        std::string lbl = childLabel(pdir, folderPath);
        wxTreeItemId nid =
            m_tree->AppendItem(pit->second, lbl, -1, -1, new RegistryTreePathData(folderPath));
        nodeByPath[folderPath] = nid;
    }

    m_tree->Expand(root);
}

void RegistryBody::populateProperties(const std::string& nodePath) {
    if (!m_list)
        return;
    m_list->DeleteAllItems();
    m_propertyRowFullPaths.clear();

    const auto data = RegistryDb::getInstance().snapshotStrings();
    const std::string nk = reg::normalizeDualLookupKey(nodePath);
    std::vector<std::string> kids = reg::listChildKeys(data, nk, true);

    long idx = 0;
    for (const std::string& fk : kids) {
        std::string name = childLabel(nk, fk);
        auto it = data.find(fk);
        bool leaf = it != data.end();
        m_propertyRowFullPaths.push_back(fk);
        m_list->InsertItem(idx, name);
        m_list->SetItem(idx, 1, leaf ? it->second : "(folder)");
        ++idx;
    }
}

void RegistryBody::editSelectedProperty() {
    if (!m_list)
        return;
    long item = -1;
    item = m_list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item == -1)
        return;
    if (item < 0 || static_cast<size_t>(item) >= m_propertyRowFullPaths.size())
        return;

    wxString val = m_list->GetItemText(item, 1);
    const std::string fullPath = m_propertyRowFullPaths[static_cast<size_t>(item)];

    if (val == "(folder)") {
        std::function<void(wxTreeItemId)> dfs;
        dfs = [&](wxTreeItemId id) {
            if (!id.IsOk())
                return;
            auto* d = dynamic_cast<RegistryTreePathData*>(m_tree->GetItemData(id));
            if (d && d->path == fullPath) {
                m_tree->SelectItem(id);
                return;
            }
            wxTreeItemIdValue cookie;
            for (wxTreeItemId ch = m_tree->GetFirstChild(id, cookie); ch.IsOk();
                 ch = m_tree->GetNextChild(id, cookie)) {
                dfs(ch);
            }
        };
        dfs(m_tree->GetRootItem());
        return;
    }

    wxTextEntryDialog dlg(m_frame, "Value:", "Edit Registry Value", val);
    if (dlg.ShowModal() != wxID_OK)
        return;
    wxString newVal = dlg.GetValue();

    RegistryDb::getInstance().set(fullPath, newVal.ToStdString());
    RegistryDb::getInstance().save();
    populateProperties(m_selectedPath);
}

} // namespace os


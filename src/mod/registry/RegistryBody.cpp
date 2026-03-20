#include "RegistryBody.hpp"

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

RegistryBody::RegistryBody() {
    std::string dir = "heroicons/normal";

    group(ID_GROUP_REG, "file", "registry", 1000, "&Registry", "Registry actions").install();
    action(ID_RELOAD, "file/registry", "reload", 0, "&Reload", "Reload registry")
        .icon(wxART_REDO, dir, "arrow-path.svg")
        .performFn([this](PerformContext* ctx) { onReload(ctx); })
        .install();
}

void RegistryBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    RegistryDb::getInstance().load();

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
}

wxEvtHandler* RegistryBody::getEventHandler() {
    return m_tree ? m_tree->GetEventHandler() : nullptr;
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
                    m_tree->AppendItem(parent, part, -1, -1, new RegistryTreePathData(next));
                nodeByPath[next] = nid;
                parent = nid;
            } else {
                parent = it->second;
            }
            path = next;
        }
    }

    m_tree->Expand(root);
}

void RegistryBody::populateProperties(const std::string& nodePath) {
    if (!m_list)
        return;
    m_list->DeleteAllItems();

    const auto data = RegistryDb::getInstance().snapshotStrings();
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
        m_list->InsertItem(idx, name);
        m_list->SetItem(idx, 1, e.hasValue ? e.value : "(folder)");
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

    wxString name = m_list->GetItemText(item, 0);
    wxString val = m_list->GetItemText(item, 1);
    if (val == "(folder)") {
        const std::string next = m_selectedPath.empty() ? name.ToStdString()
                                                       : (m_selectedPath + "." + name.ToStdString());
        wxTreeItemId root = m_tree->GetRootItem();
        std::vector<std::string> parts = splitKey(next);
        wxTreeItemId cur = root;
        for (const auto& p : parts) {
            wxTreeItemIdValue cookie;
            wxTreeItemId child = m_tree->GetFirstChild(cur, cookie);
            bool found = false;
            while (child.IsOk()) {
                if (m_tree->GetItemText(child).ToStdString() == p) {
                    cur = child;
                    found = true;
                    break;
                }
                child = m_tree->GetNextChild(cur, cookie);
            }
            if (!found)
                break;
        }
        if (cur.IsOk())
            m_tree->SelectItem(cur);
        return;
    }

    const std::string fullKey =
        m_selectedPath.empty() ? name.ToStdString() : (m_selectedPath + "." + name.ToStdString());

    wxTextEntryDialog dlg(m_frame, "Value:", "Edit Registry Value", val);
    if (dlg.ShowModal() != wxID_OK)
        return;
    wxString newVal = dlg.GetValue();

    RegistryDb::getInstance().set(fullKey, newVal.ToStdString());
    RegistryDb::getInstance().save();
    populateProperties(m_selectedPath);
}

} // namespace os


#include "ExplorerBody.hpp"

#include "../../ui/Location.hpp"
#include "../../ui/widget/BreadcrumbNav.hpp"
#include "../../ui/widget/FileListView.hpp"
#include "../../ui/widget/DirTreeView.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>

namespace {

enum {
    ID_GROUP_FILE = uiFrame::ID_APP_HIGHEST + 320,
    ID_GROUP_EDIT,
    ID_GROUP_VIEW,
    ID_GROUP_TOOLS,
    ID_GROUP_HELP,
};

} // namespace

ExplorerBody::ExplorerBody(VolumeManager* vm)
    : m_vm(vm) {
}

ExplorerBody::~ExplorerBody() = default;

void ExplorerBody::setOpenTarget(Volume* volume, const std::string& dir) {
    m_volume = volume;
    m_dir = dir.empty() ? "/" : normalizePath(dir);
}

void ExplorerBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    m_panel = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());

    Volume* vol = m_volume ? m_volume : (m_vm ? m_vm->getDefaultVolume() : nullptr);
    if (!vol) {
        m_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
        return;
    }
    if (!ensureVolumeAccess(vol)) {
        wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
        s->Add(new wxStaticText(m_panel, wxID_ANY, "Unable to authenticate selected volume."), 0, wxALL, 8);
        m_panel->SetSizer(s);
        return;
    }
    m_locationHistory = std::make_unique<LocationHistory>(vol, normalizePath(m_dir));
    Location location = m_locationHistory->location();

    createActions();

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    m_breadcrumbNav = new BreadcrumbNav(m_panel, location.path);
    m_splitter = new wxSplitterWindow(m_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D | wxSP_LIVE_UPDATE);
    m_dirTreeView = new DirTreeView(m_splitter, location);
    m_fileListView = new FileListView(m_splitter, location);
    m_splitter->SplitVertically(m_dirTreeView, m_fileListView, 300);
    m_splitter->SetMinimumPaneSize(200);

    m_statusLine = new wxStaticText(m_panel, wxID_ANY, "Ready", wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);

    mainSizer->Add(m_breadcrumbNav, 0, wxEXPAND | wxALL, 2);
    mainSizer->Add(m_splitter, 1, wxEXPAND);
    mainSizer->Add(m_statusLine, 0, wxEXPAND | wxALL, 2);
    m_panel->SetSizer(mainSizer);

    setupCallbacks();
}

wxEvtHandler* ExplorerBody::getEventHandler() {
    return m_panel ? m_panel->GetEventHandler() : nullptr;
}

void ExplorerBody::createActions() {
    group(ID_GROUP_FILE, "", "file").label("&File").description("File operations").install();
    group(ID_GROUP_EDIT, "", "edit").label("&Edit").description("Edit operations").install();
    group(ID_GROUP_VIEW, "", "view").label("&View").description("View operations").install();
    group(ID_GROUP_TOOLS, "", "tools").label("&Tools").description("Tools operations").install();
    group(ID_GROUP_HELP, "", "help").label("&Help").description("Help operations").install();

    action(wxID_BACKWARD, "view", "back")
        .label("Back").description("Go back").icon(wxART_GO_BACK).addShortcut("Alt+Left")
        .performFn([this](PerformContext* c) { doGoBack(c); }).install();
    action(wxID_FORWARD, "view", "forward")
        .label("Forward").description("Go forward").icon(wxART_GO_FORWARD).addShortcut("Alt+Right")
        .performFn([this](PerformContext* c) { doGoForward(c); }).install();
    action(wxID_UP, "view", "up")
        .label("Up").description("Go up one level").icon(wxART_GO_DIR_UP).addShortcut("Alt+Up")
        .performFn([this](PerformContext* c) { doGoUp(c); }).install();
    action(wxID_HOME, "view", "home")
        .label("Home").description("Go to home directory").icon(wxART_GO_HOME).addShortcut("Alt+Home")
        .performFn([this](PerformContext* c) { doGoHome(c); }).install();
    action(wxID_VIEW_DETAILS, "view", "list")
        .label("List View").description("Show list").icon(wxART_LIST_VIEW)
        .performFn([this](PerformContext* c) { doSetListViewMode(c); }).install();
    action(wxID_VIEW_SMALLICONS, "view", "grid")
        .label("Grid View").description("Show grid").icon(wxART_REPORT_VIEW).addShortcut("Ctrl+2")
        .performFn([this](PerformContext* c) { doSetGridViewMode(c); }).install();
    action(wxID_REFRESH, "view", "refresh")
        .label("Refresh").description("Refresh view").icon(wxART_MAKE_ART_ID("wxART_REFRESH")).addShortcut("F5")
        .performFn([this](PerformContext* c) { doRefresh(c); }).install();

    action(wxID_COPY, "edit", "copy")
        .label("Copy").description("Copy selected").icon(wxART_COPY).addShortcut("Ctrl+C")
        .performFn([this](PerformContext* c) { doCopy(c); }).install();
    action(wxID_CUT, "edit", "cut")
        .label("Cut").description("Cut selected").icon(wxART_CUT).addShortcut("Ctrl+X")
        .performFn([this](PerformContext* c) { doCut(c); }).install();
    action(wxID_PASTE, "edit", "paste")
        .label("Paste").description("Paste").icon(wxART_PASTE).addShortcut("Ctrl+V")
        .performFn([this](PerformContext* c) { doPaste(c); }).install();
    action(wxID_DELETE, "edit", "delete")
        .label("Delete").description("Delete selected").icon(wxART_DELETE).addShortcut("Del")
        .performFn([this](PerformContext* c) { doDelete(c); }).install();
}

void ExplorerBody::setupCallbacks() {
    if (!m_locationHistory || !m_breadcrumbNav || !m_dirTreeView || !m_fileListView) return;

    m_locationHistory->onLocationChange([this](const Location& location) {
        if (!ensureVolumeAccess(location.volume)) return;
        if (m_dirTreeView) m_dirTreeView->setLocation(location);
        if (m_fileListView) m_fileListView->setLocation(location);
        if (m_breadcrumbNav) m_breadcrumbNav->setLocation(location.path);
        if (m_statusLine) m_statusLine->SetLabel("Path: " + location.toString());
    });

    m_breadcrumbNav->onPathSelected([this](const std::string& path) {
        m_locationHistory->go(m_locationHistory->location().volume, path);
    });

    m_dirTreeView->onLocationChange([this](const Location& loc) {
        m_locationHistory->go(loc.volume, loc.path);
    });

    m_fileListView->onFileSelected([this](VolumeFile&) { updateStatusLine(); });
    m_fileListView->onFileActivated([this](VolumeFile& f) {
        std::string p(f.getPath());
        size_t pos = p.find_last_of('/');
        std::string filename = (pos == std::string::npos) ? p : p.substr(pos + 1);
        if (m_panel) m_panel->CallAfter([this, filename]() { openFile(filename); });
    });
    m_fileListView->onSelectionChanged([this](const std::vector<VolumeFile>&) { updateStatusLine(); });
}

void ExplorerBody::updateStatusLine() {
    if (!m_statusLine || !m_fileListView) return;
    auto sel = m_fileListView->getSelectedFiles();
    if (sel.empty()) m_statusLine->SetLabel("Ready");
    else if (sel.size() == 1) m_statusLine->SetLabel("Selected: " + sel[0]);
    else m_statusLine->SetLabel("Selected: " + std::to_string(sel.size()) + " files");
}

void ExplorerBody::refresh() {
    if (m_dirTreeView) m_dirTreeView->refresh();
    if (m_fileListView) m_fileListView->refresh();
}

void ExplorerBody::setViewMode(const std::string& mode) {
    m_viewMode = mode;
    if (m_fileListView) m_fileListView->setViewMode(mode);
}

bool ExplorerBody::ensureVolumeAccess(Volume* volume) {
    (void)volume;
    return true;
}

void ExplorerBody::openFile(const std::string& filename) {
    if (!m_locationHistory) return;
    Volume* vol = m_locationHistory->location().volume;
    std::string base = m_locationHistory->location().path;
    std::string fullPath = (base.empty() || base == "/") ? ("/" + filename) : (base + "/" + filename);
    if (vol->isDirectory(fullPath)) {
        m_locationHistory->go(vol, fullPath);
        return;
    }
    if (!vol->isFile(fullPath)) return;
    wxMessageBox("File viewer is not available in Explorer yet.", "Explorer", wxOK | wxICON_INFORMATION);
}

int ExplorerBody::doGoBack(PerformContext* c) {
    (void)c;
    if (m_locationHistory) m_locationHistory->goBack();
    return 0;
}
int ExplorerBody::doGoForward(PerformContext* c) {
    (void)c;
    if (m_locationHistory) m_locationHistory->goForward();
    return 0;
}
int ExplorerBody::doGoUp(PerformContext* c) {
    (void)c;
    if (m_locationHistory) m_locationHistory->goUp();
    return 0;
}
int ExplorerBody::doGoHome(PerformContext* c) {
    (void)c;
    if (m_locationHistory) m_locationHistory->goHome();
    return 0;
}
int ExplorerBody::doSetListViewMode(PerformContext* c) {
    (void)c;
    setViewMode("list");
    return 0;
}
int ExplorerBody::doSetGridViewMode(PerformContext* c) {
    (void)c;
    setViewMode("grid");
    return 0;
}
int ExplorerBody::doRefresh(PerformContext* c) {
    (void)c;
    refresh();
    return 0;
}
int ExplorerBody::doCopy(PerformContext* c) {
    (void)c;
    if (!m_fileListView) return -1;
    if (m_fileListView->getSelectedFiles().empty()) {
        wxMessageBox("No files selected", "Info", wxOK | wxICON_INFORMATION);
        return -1;
    }
    return 0;
}
int ExplorerBody::doCut(PerformContext* c) {
    (void)c;
    wxMessageBox("Cut not implemented yet", "Info", wxOK | wxICON_INFORMATION);
    return 0;
}
int ExplorerBody::doPaste(PerformContext* c) {
    (void)c;
    wxMessageBox("Paste not implemented yet", "Info", wxOK | wxICON_INFORMATION);
    return 0;
}
int ExplorerBody::doDelete(PerformContext* c) {
    (void)c;
    if (!m_fileListView || !m_locationHistory) return -1;
    auto sel = m_fileListView->getSelectedFiles();
    if (sel.empty()) {
        wxMessageBox("No files selected", "Info", wxOK | wxICON_INFORMATION);
        return -1;
    }
    for (const auto& file : sel) {
        Location loc = m_locationHistory->location();
        std::unique_ptr<VolumeFile> vf = loc.join(file);
        if (vf->isFile())
            vf->removeFile();
        else if (vf->isDirectory())
            vf->removeDirectory();
    }
    refresh();
    return 0;
}

#ifndef SOS_MOD_SOS_EXPLORER_CORE_HPP
#define SOS_MOD_SOS_EXPLORER_CORE_HPP

#include "../../ui/LocationHistory.hpp"
#include "../../ui/widget/BreadcrumbNav.hpp"
#include "../../ui/widget/DirTreeView.hpp"
#include "../../ui/widget/FileListView.hpp"

#include <bas/ui/arch/UIAction.hpp>
#include <bas/ui/arch/UIFragment.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/defs.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/stattext.h>

#include <memory>
#include <string>

/**
 * Explorer Body: reusable UI fragment for browsing volumes/directories.
 * Can be embedded in a Frame (standalone window) or in a Panel (e.g. tab).
 * Follows bas-ui: UIFragment + UIAction; state (Navigator, view mode) lives in Body.
 */
class ExplorerBody : public UIFragment {
public:
    explicit ExplorerBody(VolumeManager* vm);
    ~ExplorerBody() override;

    /** Set initial volume and path (call before createFragmentView when running as standalone). */
    void setOpenTarget(Volume* volume, const std::string& dir);

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

private:
    void createActions();
    void setupCallbacks();
    void updateStatusLine();
    void refresh();
    void setViewMode(const std::string& mode);
    void openFile(const std::string& filename);
    bool ensureVolumeAccess(Volume* volume);

    int doGoBack(PerformContext* c);
    int doGoForward(PerformContext* c);
    int doGoUp(PerformContext* c);
    int doGoHome(PerformContext* c);
    int doSetListViewMode(PerformContext* c);
    int doSetGridViewMode(PerformContext* c);
    int doRefresh(PerformContext* c);
    int doCopy(PerformContext* c);
    int doCut(PerformContext* c);
    int doPaste(PerformContext* c);
    int doDelete(PerformContext* c);

    VolumeManager* m_vm{nullptr};
    Volume* m_volume{nullptr};
    std::string m_dir{"/"};
    std::string m_viewMode{"list"};

    std::unique_ptr<LocationHistory> m_locationHistory;
    wxPanel* m_panel{nullptr};
    BreadcrumbNav* m_breadcrumbNav{nullptr};
    wxSplitterWindow* m_splitter{nullptr};
    DirTreeView* m_dirTreeView{nullptr};
    FileListView* m_fileListView{nullptr};
    wxStaticText* m_statusLine{nullptr};
};

#endif

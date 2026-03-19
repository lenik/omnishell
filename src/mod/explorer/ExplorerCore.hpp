#ifndef OMNISHELL_MOD_EXPLORER_CORE_HPP
#define OMNISHELL_MOD_EXPLORER_CORE_HPP

#include <bas/ui/arch/ImageSet.hpp>
#include <bas/ui/arch/UIFragment.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/treectrl.h>

#include <string>

class VolumeManager;
struct VolumeFile;

namespace os {

class ExplorerCore : public UIFragment {
public:
    ExplorerCore(VolumeManager* vm, Volume* volume, std::string dir);
    ~ExplorerCore() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

    void setDir(std::string dir);
    const std::string& dir() const { return dir_; }

private:
    class TreeData : public wxTreeItemData {
    public:
        TreeData(Volume* v, std::string p) : volume(v), path(std::move(p)) {}
        Volume* volume{nullptr};
        std::string path;
        bool loaded{false};
    };

    VolumeManager* vm_{nullptr};
    Volume* volume_{nullptr};
    std::string dir_;

    uiFrame* frame_{nullptr};
    wxStaticText* pathLabel_{nullptr};
    wxTreeCtrl* tree_{nullptr};
    wxListCtrl* list_{nullptr};

    // Icons for tree and list
    wxImageList* treeImages_{nullptr};
    wxImageList* listImages_{nullptr};
    int volIcon_[static_cast<int>(VolumeType::OTHER) + 1] = {};
    int iconFolder_{-1};
    int iconFileText_{-1};
    int iconFileImage_{-1};
    int iconFileAudio_{-1};
    int iconFileVideo_{-1};
    int iconFilePdf_{-1};
    int iconFileCode_{-1};
    int iconFileArchive_{-1};
    int iconFileSheet_{-1};
    int iconFileGeneric_{-1};

    int listIconForExtension(const std::string& ext) const;

    void refresh();
    void refreshTreeVolumes();
    void loadTreeChildren(const wxTreeItemId& id);
    void selectTreePath(Volume* vol, const std::string& path);

    void openChild(const std::string& name, bool isDir);
    void openFile(const std::string& name);

    void onUp(PerformContext* ctx);
    void onRefresh(PerformContext* ctx);
};

} // namespace os

#endif


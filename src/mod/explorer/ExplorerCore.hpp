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
    const std::string& dir() const { return m_dir; }

private:
    class TreeData : public wxTreeItemData {
    public:
        TreeData(Volume* v, std::string p) : volume(v), path(std::move(p)) {}
        Volume* volume{nullptr};
        std::string path;
        bool loaded{false};
    };

    VolumeManager* m_vm{nullptr};
    Volume* m_volume{nullptr};
    std::string m_dir;

    uiFrame* m_frame{nullptr};
    wxStaticText* m_pathLabel{nullptr};
    wxTreeCtrl* m_tree{nullptr};
    wxListCtrl* m_list{nullptr};

    // Icons for tree and list
    wxImageList* m_treeImages{nullptr};
    wxImageList* m_listImages{nullptr};
    int m_volIcon[static_cast<int>(VolumeType::OTHER) + 1] = {};
    int m_iconFolder{-1};
    int m_iconFileText{-1};
    int m_iconFileImage{-1};
    int m_iconFileAudio{-1};
    int m_iconFileVideo{-1};
    int m_iconFilePdf{-1};
    int m_iconFileCode{-1};
    int m_iconFileArchive{-1};
    int m_iconFileSheet{-1};
    int m_iconFileGeneric{-1};

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


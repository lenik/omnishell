#ifndef OMNISHELL_MOD_REGISTRY_CORE_HPP
#define OMNISHELL_MOD_REGISTRY_CORE_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/treectrl.h>

namespace os {

class RegistryTreePathData : public wxTreeItemData {
public:
    explicit RegistryTreePathData(std::string p) : path(std::move(p)) {}
    std::string path;
};

class RegistryCore : public UIFragment {
public:
    RegistryCore();
    ~RegistryCore() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

private:
    uiFrame* m_frame{nullptr};
    wxTreeCtrl* m_tree{nullptr};
    wxListCtrl* m_list{nullptr};
    std::string m_selectedPath;

    void buildTree();
    void populateProperties(const std::string& nodePath);
    void editSelectedProperty();

    void onReload(PerformContext* ctx);
};

} // namespace os

#endif


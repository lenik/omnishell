#ifndef OMNISHELL_MOD_CONTROLPANEL_BODY_HPP
#define OMNISHELL_MOD_CONTROLPANEL_BODY_HPP

#include "../../core/App.hpp"
#include "../../core/Module.hpp"

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/listctrl.h>
#include <wx/wx.h>

#include <vector>

namespace os {

class ControlPanelBody : public UIFragment {
  public:
    explicit ControlPanelBody(App* app);
    ~ControlPanelBody() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

  private:
    void OnCategorySelected(wxCommandEvent& event);
    void OnModuleDoubleClicked(wxCommandEvent& event);

    void showModuleManager();
    void showSystemSettings();
    void showUserConfiguration();
    void showDesktopSettings();
    void showDisplaySettings();
    void showAbout();

    uiFrame* m_frame{nullptr};
    wxListView* m_categoryList{nullptr};
    wxPanel* m_contentPanel{nullptr};

    enum class PanelCategory {
        ModuleManager,
        SystemSettings,
        UserConfiguration,
        Desktop,
        Display,
        About,
    };

    struct Category {
        PanelCategory id;
        wxString name;
        wxString description;
        int imageIndex;
    };

    std::vector<Category> m_categories;
};

} // namespace os

#endif

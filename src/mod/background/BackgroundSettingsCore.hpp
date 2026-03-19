#ifndef OMNISHELL_MOD_BACKGROUND_SETTINGS_CORE_HPP
#define OMNISHELL_MOD_BACKGROUND_SETTINGS_CORE_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/clrpicker.h>
#include <wx/panel.h>

namespace os {

class BackgroundSettingsCore : public UIFragment {
public:
    BackgroundSettingsCore();
    ~BackgroundSettingsCore() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

private:
    uiFrame* frame_{nullptr};
    wxPanel* root_{nullptr};
    wxColourPickerCtrl* picker_{nullptr};
    std::string selectedImagePath_;

    void onChooseImage(PerformContext* ctx);
    void onApplyColor(PerformContext* ctx);
    void onApplyImage(PerformContext* ctx);
};

} // namespace os

#endif


#ifndef OMNISHELL_MOD_BACKGROUND_SETTINGS_CORE_HPP
#define OMNISHELL_MOD_BACKGROUND_SETTINGS_CORE_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/clrpicker.h>
#include <wx/panel.h>

namespace os {

class BackgroundSettingsBody : public UIFragment {
public:
    BackgroundSettingsBody();
    ~BackgroundSettingsBody() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

private:
    uiFrame* m_frame{nullptr};
    wxPanel* m_root{nullptr};
    wxColourPickerCtrl* m_picker{nullptr};
    std::string m_selectedImagePath;
    std::string m_selectedImageVolumeId;

    void onChooseImage(PerformContext* ctx);
    void onApplyColor(PerformContext* ctx);
    void onApplyImage(PerformContext* ctx);
};

} // namespace os

#endif


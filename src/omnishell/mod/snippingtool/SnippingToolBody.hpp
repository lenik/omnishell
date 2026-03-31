#ifndef OMNISHELL_MOD_SNIPPINGTOOL_BODY_HPP
#define OMNISHELL_MOD_SNIPPINGTOOL_BODY_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/bitmap.h>

class wxChoice;
class wxPanel;
class wxStaticBitmap;

namespace os {

class SnippingToolBody : public UIFragment {
public:
    SnippingToolBody() = default;
    ~SnippingToolBody() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

private:
    uiFrame* m_frame{nullptr};
    wxPanel* m_panel{nullptr};
    wxStaticBitmap* m_preview{nullptr};
    wxChoice* m_modeChoice{nullptr};
    wxBitmap m_capturedBitmap;

    void OnNewSnip(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnMode(wxCommandEvent& event);
};

} // namespace os

#endif

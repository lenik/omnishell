#ifndef OMNISHELL_MOD_BROWSER_BODY_HPP
#define OMNISHELL_MOD_BROWSER_BODY_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>
#include <wx/event.h>

#include <wx/string.h>

#include <vector>

class VolumeManager;
class wxWebView;
class wxComboBox;
class wxButton;

namespace os {

class BrowserBody : public UIFragment {
  public:
    explicit BrowserBody(VolumeManager* vm);
    ~BrowserBody() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

    void navigateTo(const wxString& url);

  private:
    void onGo();
    void onHistoryPick(wxCommandEvent&);
    void onUrlComboEnter(wxCommandEvent&);
    void pushHistory(const wxString& url);

    uiFrame* m_frame = nullptr;
    VolumeManager* m_vm;
    wxComboBox* m_urlCombo = nullptr;
    wxWebView* m_web = nullptr;
    wxButton* m_btnBack = nullptr;
    wxButton* m_btnFwd = nullptr;
    std::vector<wxString> m_history;
    static constexpr size_t kMaxHistory = 40;
    bool m_updatingCombo = false;
};

} // namespace os

#endif

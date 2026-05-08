#ifndef OMNISHELL_MOD_CALCULATOR_BODY_HPP
#define OMNISHELL_MOD_CALCULATOR_BODY_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/string.h>

class wxPanel;
class wxTextCtrl;

namespace os {

class CalculatorBody : public UIFragment {
public:
    CalculatorBody() = default;
    ~CalculatorBody() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

private:
    uiFrame* m_frame{nullptr};
    wxPanel* m_panel{nullptr};
    wxTextCtrl* m_display{nullptr};

    wxString m_currentInput{wxS("0")};
    wxString m_previousInput;
    wxString m_operation;
    bool m_newInput{true};

    void OnButton(wxCommandEvent& event);
    void OnKeyPress(wxKeyEvent& event);
    void updateDisplay();
    void calculate();
};

} // namespace os

#endif

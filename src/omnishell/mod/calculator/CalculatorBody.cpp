#include "CalculatorBody.hpp"

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

namespace os {

namespace {
enum {
    ID_BTN_0 = wxID_HIGHEST + 1,
    ID_BTN_1,
    ID_BTN_2,
    ID_BTN_3,
    ID_BTN_4,
    ID_BTN_5,
    ID_BTN_6,
    ID_BTN_7,
    ID_BTN_8,
    ID_BTN_9,
    ID_BTN_ADD,
    ID_BTN_SUB,
    ID_BTN_MUL,
    ID_BTN_DIV,
    ID_BTN_EQ,
    ID_BTN_CLR,
    ID_BTN_DOT,
    ID_BTN_NEG
};
}

void CalculatorBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    m_panel = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    m_panel->SetMinSize(wxSize(280, 380));

    auto* mainsizer = new wxBoxSizer(wxVERTICAL);

    m_display = new wxTextCtrl(m_panel, wxID_ANY, wxS("0"), wxDefaultPosition, wxDefaultSize,
                               wxTE_RIGHT | wxTE_READONLY);
    m_display->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    mainsizer->Add(m_display, 0, wxEXPAND | wxALL, 10);

    auto* btnsizer = new wxGridSizer(5, 4, 5, 5);

    btnsizer->Add(new wxButton(m_panel, ID_BTN_CLR, wxS("C")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_NEG, wxS("+/-")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_DIV, wxS("\u00F7")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_MUL, wxS("\u00D7")), 0, wxEXPAND);

    btnsizer->Add(new wxButton(m_panel, ID_BTN_7, wxS("7")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_8, wxS("8")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_9, wxS("9")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_SUB, wxS("\u2212")), 0, wxEXPAND);

    btnsizer->Add(new wxButton(m_panel, ID_BTN_4, wxS("4")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_5, wxS("5")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_6, wxS("6")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_ADD, wxS("+")), 0, wxEXPAND);

    btnsizer->Add(new wxButton(m_panel, ID_BTN_1, wxS("1")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_2, wxS("2")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_3, wxS("3")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_EQ, wxS("=")), 0, wxEXPAND);

    btnsizer->Add(new wxButton(m_panel, ID_BTN_0, wxS("0")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, wxID_ANY, wxS("00")), 0, wxEXPAND);
    btnsizer->Add(new wxButton(m_panel, ID_BTN_DOT, wxS(".")), 0, wxEXPAND);

    mainsizer->Add(btnsizer, 1, wxEXPAND | wxALL, 10);
    m_panel->SetSizer(mainsizer);

    m_panel->Bind(wxEVT_BUTTON, &CalculatorBody::OnButton, this);
    if (m_frame)
        m_frame->Bind(wxEVT_KEY_DOWN, &CalculatorBody::OnKeyPress, this);
}

wxEvtHandler* CalculatorBody::getEventHandler() {
    if (m_frame)
        return m_frame->GetEventHandler();
    return m_panel ? m_panel->GetEventHandler() : nullptr;
}

void CalculatorBody::OnButton(wxCommandEvent& event) {
    wxObject* obj = event.GetEventObject();
    wxButton* btn = wxDynamicCast(obj, wxButton);
    if (!btn)
        return;

    wxString label = btn->GetLabel();

    if (label >= wxS("0") && label <= wxS("9")) {
        if (m_newInput) {
            m_currentInput = label;
            m_newInput = false;
        } else {
            if (m_currentInput == wxS("0"))
                m_currentInput = label;
            else
                m_currentInput += label;
        }
    } else if (label == wxS("00")) {
        if (m_newInput) {
            m_currentInput = wxS("0");
            m_newInput = false;
        } else {
            m_currentInput += wxS("00");
        }
    } else if (label == wxS(".")) {
        if (m_newInput) {
            m_currentInput = wxS("0.");
            m_newInput = false;
        } else if (m_currentInput.Find('.') == wxNOT_FOUND) {
            m_currentInput += wxS(".");
        }
    } else if (label == wxS("C")) {
        m_currentInput = wxS("0");
        m_previousInput.clear();
        m_operation.clear();
        m_newInput = true;
    } else if (label == wxS("+/-")) {
        if (!m_currentInput.empty() && m_currentInput[0] == wxS('-'))
            m_currentInput = m_currentInput.Mid(1);
        else
            m_currentInput = wxS("-") + m_currentInput;
    } else if (label == wxS("+") || label == wxS("\u2212") || label == wxS("\u00D7") ||
               label == wxS("\u00F7")) {
        m_previousInput = m_currentInput;
        m_operation = label;
        m_newInput = true;
    } else if (label == wxS("=")) {
        calculate();
        m_operation.clear();
        m_newInput = true;
    }

    updateDisplay();
}

void CalculatorBody::OnKeyPress(wxKeyEvent& event) {
    event.Skip();
}

void CalculatorBody::updateDisplay() {
    if (m_display)
        m_display->SetValue(m_currentInput);
}

void CalculatorBody::calculate() {
    if (m_operation.empty() || m_previousInput.empty())
        return;

    double prev = 0.0, curr = 0.0, result = 0.0;
    m_previousInput.ToDouble(&prev);
    m_currentInput.ToDouble(&curr);

    if (m_operation == wxS("+"))
        result = prev + curr;
    else if (m_operation == wxS("\u2212"))
        result = prev - curr;
    else if (m_operation == wxS("\u00D7"))
        result = prev * curr;
    else if (m_operation == wxS("\u00F7") && curr != 0.0)
        result = prev / curr;

    m_currentInput = wxString::Format(wxS("%g"), result);
}

} // namespace os

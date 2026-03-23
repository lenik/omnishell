#include "LoginDialog.hpp"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace os {

LoginDialog::LoginDialog(wxWindow* parent, const wxString& title, const wxString& message)
    : wxcDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    auto* root = new wxBoxSizer(wxVERTICAL);
    if (!message.IsEmpty()) {
        root->Add(new wxStaticText(this, wxID_ANY, message), 0, wxALL, 8);
    }
    root->Add(new wxStaticText(this, wxID_ANY, _("User name:")), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    m_user = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(320, -1));
    root->Add(m_user, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    root->Add(new wxStaticText(this, wxID_ANY, _("Password:")), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    m_pass = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(320, -1),
                            wxTE_PASSWORD);
    root->Add(m_pass, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    auto* btnSizer = new wxStdDialogButtonSizer();
    btnSizer->AddButton(new wxButton(this, wxID_OK));
    btnSizer->AddButton(new wxButton(this, wxID_CANCEL));
    btnSizer->Realize();
    root->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 8);
    SetSizerAndFit(root);
    CentreOnParent();
}

std::string LoginDialog::username() const {
    return m_user ? std::string(m_user->GetValue().ToUTF8()) : std::string();
}

std::string LoginDialog::password() const {
    return m_pass ? std::string(m_pass->GetValue().ToUTF8()) : std::string();
}

} // namespace os

#ifndef OMNISHELL_MOD_PASSWORDMANAGER_LOGIN_DIALOG_HPP
#define OMNISHELL_MOD_PASSWORDMANAGER_LOGIN_DIALOG_HPP

#include "../../wx/wxcWindow.hpp"

#include <wx/dialog.h>

#include <string>

class wxTextCtrl;

namespace os {

class LoginDialog : public wxcDialog {
  public:
    LoginDialog(wxWindow* parent, const wxString& title, const wxString& message = {});

    std::string username() const;
    std::string password() const;

  private:
    wxTextCtrl* m_user = nullptr;
    wxTextCtrl* m_pass = nullptr;
};

} // namespace os

#endif

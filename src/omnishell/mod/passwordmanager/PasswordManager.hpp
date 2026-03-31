#ifndef OMNISHELL_MOD_PASSWORDMANAGER_PASSWORD_MANAGER_HPP
#define OMNISHELL_MOD_PASSWORDMANAGER_PASSWORD_MANAGER_HPP

#include "PasswordDb.hpp"

#include "../../core/auth/IAuthProxy.hpp"

#include <wx/window.h>

namespace os {

/**
 * IAuthProxy that reuses stored credentials per requestUri or prompts with LoginDialog,
 * then encodes/submits via the given AuthContext until success or attempts exhausted.
 */
class PasswordManager : public auth::IAuthProxy {
  public:
    explicit PasswordManager(wxWindow* dialogParent = nullptr, int maxAttempts = 3);

    auth::AuthResult authenticate(auth::AuthContext& context) override;

    PasswordDb& passwordDb() { return m_passwordDb; }
    const PasswordDb& passwordDb() const { return m_passwordDb; }

    void setMaxAttempts(int n) { m_maxAttempts = n; }

  private:
    wxWindow* m_dialogParent;
    int m_maxAttempts;
    PasswordDb m_passwordDb;
};

} // namespace os

#endif

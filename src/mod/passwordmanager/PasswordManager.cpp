#include "PasswordManager.hpp"

#include "LoginDialog.hpp"

#include "../../core/auth/AuthContext.hpp"

namespace os {

PasswordManager::PasswordManager(wxWindow* dialogParent, int maxAttempts)
    : m_dialogParent(dialogParent)
    , m_maxAttempts(maxAttempts) {}

auth::AuthResult PasswordManager::authenticate(auth::AuthContext& context) {
    int remaining = m_maxAttempts;
    while (remaining-- > 0) {
        auth::AuthData data;
        if (m_passwordDb.contains(context.requestUri)) {
            data = m_passwordDb.get(context.requestUri);
        } else {
            LoginDialog dlg(m_dialogParent, _("Sign in"),
                            wxString::FromUTF8(context.source.data(), context.source.size()));
            if (dlg.ShowModal() != wxID_OK) {
                return auth::AuthResult::failure();
            }
            data.userName = dlg.username();
            data.password = dlg.password();
        }

        std::string payload = context.encode(data);
        auth::AuthResult result = context.submit(payload);
        if (result) {
            m_passwordDb.put(context.requestUri, data);
            if (!result.userName && !data.userName.empty()) {
                result.userName = data.userName;
            }
            return result;
        }
    }
    return auth::AuthResult::failure();
}

} // namespace os

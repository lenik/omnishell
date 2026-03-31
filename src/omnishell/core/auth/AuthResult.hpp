#ifndef OMNISHELL_CORE_AUTH_AUTH_RESULT_HPP
#define OMNISHELL_CORE_AUTH_AUTH_RESULT_HPP

#include "IAuthTokenContext.hpp"

#include <optional>
#include <string>

namespace os::auth {

/** Outcome of a login / token exchange; also exposes the issued token. */
class AuthResult : public IAuthTokenContext {
  public:
    AuthResult() = default;

    static AuthResult failure() { return AuthResult(); }

    std::optional<std::string> userName;
    std::optional<std::string> token;

    explicit operator bool() const {
        return token.has_value() && !token->empty();
    }

    std::string getAuthToken() const override {
        return token.value_or("");
    }

    bool isAuthTokenExpired() const override { return m_tokenExpired; }

    void syncAuthToken() override {
        // Hook for refresh flows; default is no-op.
    }

    void setTokenExpired(bool expired) { m_tokenExpired = expired; }

  private:
    bool m_tokenExpired = false;
};

} // namespace os::auth

#endif

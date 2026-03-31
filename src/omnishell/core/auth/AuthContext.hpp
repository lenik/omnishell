#ifndef OMNISHELL_CORE_AUTH_AUTH_CONTEXT_HPP
#define OMNISHELL_CORE_AUTH_AUTH_CONTEXT_HPP

#include "AuthResult.hpp"
#include "AuthTypes.hpp"

#include <optional>
#include <string>

namespace os::auth {

/**
 * Per-request authentication flow: caller fills routing fields; concrete types
 * implement encoding and submission (e.g. HTTP Basic, form POST, custom API).
 */
class AuthContext {
  public:
    virtual ~AuthContext() = default;

    std::string source;
    std::string requestUri;
    AuthRequestType requestType = AuthRequestType::Password;
    std::optional<std::string> username;

    virtual std::string encode(const AuthData& authData) const = 0;
    virtual AuthResult submit(const std::string& payload) = 0;
};

} // namespace os::auth

#endif

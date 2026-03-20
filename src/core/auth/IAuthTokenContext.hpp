#ifndef OMNISHELL_CORE_AUTH_I_AUTH_TOKEN_CONTEXT_HPP
#define OMNISHELL_CORE_AUTH_I_AUTH_TOKEN_CONTEXT_HPP

#include <string>

namespace os::auth {

/** Access to an auth token and its lifecycle (refresh / expiry). */
class IAuthTokenContext {
  public:
    virtual ~IAuthTokenContext() = default;

    virtual std::string getAuthToken() const = 0;
    virtual bool isAuthTokenExpired() const = 0;
    virtual void syncAuthToken() = 0;
};

} // namespace os::auth

#endif

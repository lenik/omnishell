#ifndef OMNISHELL_CORE_AUTH_I_AUTH_PROXY_HPP
#define OMNISHELL_CORE_AUTH_I_AUTH_PROXY_HPP

#include "AuthResult.hpp"

namespace os::auth {

class AuthContext;

/** Obtains credentials (cached or via UI) and drives an AuthContext to a result. */
class IAuthProxy {
  public:
    virtual ~IAuthProxy() = default;

    virtual AuthResult authenticate(AuthContext& context) = 0;
};

} // namespace os::auth

#endif

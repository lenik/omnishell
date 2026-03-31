#ifndef OMNISHELL_CORE_AUTH_AUTH_TYPES_HPP
#define OMNISHELL_CORE_AUTH_AUTH_TYPES_HPP

#include <string>

namespace os::auth {

enum class AuthRequestType { Password, Pin };

struct AuthData {
    std::string userName;
    std::string password;
};

} // namespace os::auth

#endif

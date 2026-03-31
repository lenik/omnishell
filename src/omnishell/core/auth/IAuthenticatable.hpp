#ifndef OMNISHELL_CORE_AUTH_I_AUTHENTICATABLE_HPP
#define OMNISHELL_CORE_AUTH_I_AUTHENTICATABLE_HPP

#include <string>
#include <unordered_set>

namespace os::auth {

/** Principal with identity and role checks (e.g. signed-in user). */
class IAuthenticatable {
  public:
    virtual ~IAuthenticatable() = default;

    virtual std::string getUserId() const = 0;
    virtual std::string getDisplayName() const = 0;
    virtual std::unordered_set<std::string> getRoles() const = 0;

    virtual bool isAuthenticated() const = 0;
    virtual bool hasRole(const std::string& role) const = 0;
};

} // namespace os::auth

#endif

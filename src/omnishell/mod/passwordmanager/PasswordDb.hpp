#ifndef OMNISHELL_MOD_PASSWORDMANAGER_PASSWORD_DB_HPP
#define OMNISHELL_MOD_PASSWORDMANAGER_PASSWORD_DB_HPP

#include "../../core/auth/AuthTypes.hpp"

#include <string>
#include <unordered_map>

namespace os {

/** In-memory credential store keyed by request URI (e.g. origin or API id). */
class PasswordDb {
  public:
    bool contains(const std::string& requestUri) const {
        return m_entries.find(requestUri) != m_entries.end();
    }

    auth::AuthData get(const std::string& requestUri) const {
        auto it = m_entries.find(requestUri);
        if (it != m_entries.end()) {
            return it->second;
        }
        return {};
    }

    void put(const std::string& requestUri, const auth::AuthData& data) {
        m_entries[requestUri] = data;
    }

    void erase(const std::string& requestUri) { m_entries.erase(requestUri); }

  private:
    std::unordered_map<std::string, auth::AuthData> m_entries;
};

} // namespace os

#endif

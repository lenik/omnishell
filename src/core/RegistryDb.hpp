#ifndef OMNISHELL_CORE_REGISTRY_DB_HPP
#define OMNISHELL_CORE_REGISTRY_DB_HPP

#include "registry/RegistryService.hpp"

namespace os {

/**
 * Backward-compatible accessor. Prefer registry() and IRegistry.
 */
struct RegistryDb {
    static IRegistry& getInstance() { return registry(); }
};

} // namespace os

#endif

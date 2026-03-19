#ifndef OMNISHELL_APP_REGISTRY_HPP
#define OMNISHELL_APP_REGISTRY_HPP

#include "RegistryCore.hpp"

#include "../../core/Module.hpp"

#include <wx/wx.h>

namespace os {

/**
 * Registry viewer/editor.
 *
 * Simple UI on top of RegistryDb.
 */
class RegistryApp : public Module {
public:
    explicit RegistryApp(CreateModuleContext* ctx);
    virtual ~RegistryApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

private:
    RegistryCore m_core;
};

} // namespace os

#endif // OMNISHELL_APP_REGISTRY_HPP


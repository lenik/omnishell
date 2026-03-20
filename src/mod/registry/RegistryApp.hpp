#ifndef OMNISHELL_APP_REGISTRY_HPP
#define OMNISHELL_APP_REGISTRY_HPP

#include "../../core/Module.hpp"

namespace os {

class App;

/**
 * Registry viewer/editor.
 *
 * Simple UI on top of RegistryDb (directory tree + JSON property files).
 */
class RegistryApp : public Module {
  public:
    explicit RegistryApp(CreateModuleContext* ctx);
    virtual ~RegistryApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif // OMNISHELL_APP_REGISTRY_HPP

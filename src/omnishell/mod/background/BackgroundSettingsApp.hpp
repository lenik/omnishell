#ifndef OMNISHELL_APP_BACKGROUND_SETTINGS_HPP
#define OMNISHELL_APP_BACKGROUND_SETTINGS_HPP

#include "../../core/Module.hpp"

namespace os {

class App;

class BackgroundSettingsApp : public Module {
  public:
    BackgroundSettingsApp(CreateModuleContext* ctx);
    virtual ~BackgroundSettingsApp();

    ProcessPtr run(const RunConfig& config) override;

    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif // OMNISHELL_APP_BACKGROUND_SETTINGS_HPP

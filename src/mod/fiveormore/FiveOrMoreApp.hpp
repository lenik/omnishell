#ifndef OMNISHELL_MOD_FIVE_OR_MORE_APP_HPP
#define OMNISHELL_MOD_FIVE_OR_MORE_APP_HPP

#include "../../core/Module.hpp"

namespace os {

class App;

class FiveOrMoreApp : public Module {
  public:
    explicit FiveOrMoreApp(CreateModuleContext* ctx);
    ~FiveOrMoreApp() override = default;

    ProcessPtr run() override;
    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif

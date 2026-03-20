#ifndef OMNISHELL_MOD_SOLITAIRE_APP_HPP
#define OMNISHELL_MOD_SOLITAIRE_APP_HPP

#include "../../core/Module.hpp"

namespace os {

class App;

class SolitaireApp : public Module {
  public:
    explicit SolitaireApp(CreateModuleContext* ctx);
    ~SolitaireApp() override = default;

    ProcessPtr run() override;
    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif

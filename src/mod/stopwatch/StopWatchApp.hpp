#ifndef OMNISHELL_APP_STOPWATCH_HPP
#define OMNISHELL_APP_STOPWATCH_HPP

#include "../../core/Module.hpp"

namespace os {

class App;

class StopWatchApp : public Module {
  public:
    explicit StopWatchApp(CreateModuleContext* ctx);
    virtual ~StopWatchApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif // OMNISHELL_APP_STOPWATCH_HPP

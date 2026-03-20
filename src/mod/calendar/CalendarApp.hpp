#ifndef OMNISHELL_APP_CALENDAR_HPP
#define OMNISHELL_APP_CALENDAR_HPP

#include "../../core/Module.hpp"

namespace os {

class App;

class CalendarApp : public Module {
  public:
    explicit CalendarApp(CreateModuleContext* ctx);
    virtual ~CalendarApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif // OMNISHELL_APP_CALENDAR_HPP

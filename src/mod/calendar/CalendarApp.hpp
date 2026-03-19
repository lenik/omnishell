#ifndef OMNISHELL_APP_CALENDAR_HPP
#define OMNISHELL_APP_CALENDAR_HPP

#include "CalendarCore.hpp"

#include "../../core/Module.hpp"

#include <wx/calctrl.h>
#include <wx/wx.h>

namespace os {

class CalendarApp : public Module {
public:
    explicit CalendarApp(CreateModuleContext* ctx);
    virtual ~CalendarApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

private:
    CalendarCore m_core;
};

} // namespace os

#endif // OMNISHELL_APP_CALENDAR_HPP


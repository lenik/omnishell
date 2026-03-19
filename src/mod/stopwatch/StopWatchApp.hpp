#ifndef OMNISHELL_APP_STOPWATCH_HPP
#define OMNISHELL_APP_STOPWATCH_HPP

#include "StopWatchCore.hpp"

#include "../../core/Module.hpp"

#include <wx/timer.h>
#include <wx/wx.h>

namespace os {

class StopWatchApp : public Module {
public:
    explicit StopWatchApp(CreateModuleContext* ctx);
    virtual ~StopWatchApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

private:
    StopWatchCore m_core;
};

} // namespace os

#endif // OMNISHELL_APP_STOPWATCH_HPP


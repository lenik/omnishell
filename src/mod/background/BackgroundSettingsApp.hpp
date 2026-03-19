#ifndef OMNISHELL_APP_BACKGROUND_SETTINGS_HPP
#define OMNISHELL_APP_BACKGROUND_SETTINGS_HPP

#include "BackgroundSettingsCore.hpp"

#include "../../core/Module.hpp"

#include <wx/wx.h>

namespace os {

class BackgroundSettingsApp : public Module {
public:
    BackgroundSettingsApp(CreateModuleContext* ctx);
    virtual ~BackgroundSettingsApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

private:
    BackgroundSettingsCore core_;
};

} // namespace os

#endif // OMNISHELL_APP_BACKGROUND_SETTINGS_HPP


#ifndef OMNISHELL_APP_BACKGROUND_SETTINGS_HPP
#define OMNISHELL_APP_BACKGROUND_SETTINGS_HPP

#include "BackgroundSettingsBody.hpp"

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
    BackgroundSettingsBody m_body;
};

} // namespace os

#endif // OMNISHELL_APP_BACKGROUND_SETTINGS_HPP


#ifndef OMNISHELL_APP_CONTROLPANEL_HPP
#define OMNISHELL_APP_CONTROLPANEL_HPP

#include "../../core/Module.hpp"

namespace os {

class ControlPanelFrame;

/**
 * Control Panel Application Module
 */
class ControlPanelApp : public Module {
  public:
    explicit ControlPanelApp(CreateModuleContext* ctx);
    virtual ~ControlPanelApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

  private:
    ControlPanelFrame* m_frame{nullptr};
};

} // namespace os

#endif // OMNISHELL_APP_CONTROLPANEL_HPP

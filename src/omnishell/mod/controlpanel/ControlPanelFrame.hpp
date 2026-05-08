#ifndef OMNISHELL_MOD_CONTROLPANEL_FRAME_HPP
#define OMNISHELL_MOD_CONTROLPANEL_FRAME_HPP

#include "ControlPanelBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class ControlPanelFrame : public uiFrame {
  public:
    ControlPanelFrame(App* app, std::string title);
    ~ControlPanelFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

  private:
    ControlPanelBody m_body;
};

} // namespace os

#endif

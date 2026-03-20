#ifndef OMNISHELL_MOD_BACKGROUND_SETTINGS_FRAME_HPP
#define OMNISHELL_MOD_BACKGROUND_SETTINGS_FRAME_HPP

#include "BackgroundSettingsBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class BackgroundSettingsFrame : public uiFrame {
  public:
    BackgroundSettingsFrame(App* app, std::string title);
    ~BackgroundSettingsFrame() override = default;

    void createFragmentView(CreateViewContext* ctx) override;

  private:
    BackgroundSettingsBody m_body;
};

} // namespace os

#endif

#ifndef OMNISHELL_MOD_REGISTRY_FRAME_HPP
#define OMNISHELL_MOD_REGISTRY_FRAME_HPP

#include "RegistryBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class RegistryFrame : public uiFrame {
  public:
    RegistryFrame(App* app, std::string title);
    ~RegistryFrame() override = default;

    void createFragmentView(CreateViewContext* ctx) override;

  private:
    RegistryBody m_body;
};

} // namespace os

#endif

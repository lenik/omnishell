#ifndef OMNISHELL_MOD_BROWSER_FRAME_HPP
#define OMNISHELL_MOD_BROWSER_FRAME_HPP

#include "BrowserBody.hpp"

#include <bas/wx/uiframe.hpp>

class VolumeManager;

namespace os {

class BrowserFrame : public uiFrame {
  public:
    BrowserFrame(VolumeManager* vm, std::string title);
    ~BrowserFrame() override = default;

    void createFragmentView(CreateViewContext* ctx) override;

    BrowserBody& body() { return m_body; }

  private:
    BrowserBody m_body;
};

} // namespace os

#endif

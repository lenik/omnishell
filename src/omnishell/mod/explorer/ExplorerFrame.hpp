#ifndef OMNISHELL_MOD_EXPLORER_FRAME_HPP
#define OMNISHELL_MOD_EXPLORER_FRAME_HPP

#include "ExplorerBody.hpp"

#include "../../core/App.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/wx/uiframe.hpp>

#include <string>

class VolumeManager;

namespace os {

class ExplorerFrame : public uiFrame {
  public:
    ExplorerFrame(App* app, VolumeManager* vm, Volume* volume, std::string dir);
    ~ExplorerFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

    ExplorerBody& body() { return m_body; }

  private:
    ExplorerBody m_body;
};

} // namespace os

#endif

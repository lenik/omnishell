#ifndef OMNISHELL_MOD_MEDIAPLAYER_FRAME_HPP
#define OMNISHELL_MOD_MEDIAPLAYER_FRAME_HPP

#include "MediaPlayerBody.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

class VolumeManager;

namespace os {

class MediaPlayerFrame : public uiFrame {
  public:
    MediaPlayerFrame(VolumeManager* vm, std::string title);
    ~MediaPlayerFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

    MediaPlayerBody& body() { return m_body; }

  private:
    MediaPlayerBody m_body;
};

} // namespace os

#endif

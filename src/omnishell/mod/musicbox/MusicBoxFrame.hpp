#ifndef OMNISHELL_MOD_MUSICBOX_MUSICBOXFRAME_HPP
#define OMNISHELL_MOD_MUSICBOX_MUSICBOXFRAME_HPP

#include "MusicBoxBody.hpp"

#include <bas/wx/uiframe.hpp>

namespace os {

class App;
} // namespace os

class VolumeManager;
namespace os {

class MusicBoxFrame : public uiFrame {
public:
    MusicBoxFrame(App* app, VolumeManager* vm, std::string title);
    ~MusicBoxFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

    MusicBoxBody& body() { return m_body; }

private:
    MusicBoxBody m_body;
};

} // namespace os

#endif // OMNISHELL_MOD_MUSICBOX_MUSICBOXFRAME_HPP


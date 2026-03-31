#ifndef OMNISHELL_MOD_SOUNDRECORDER_FRAME_HPP
#define OMNISHELL_MOD_SOUNDRECORDER_FRAME_HPP

#include "SoundRecorderBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class SoundRecorderFrame : public uiFrame {
public:
    SoundRecorderFrame(App* app, std::string title);
    ~SoundRecorderFrame() override = default;

    void createFragmentView(CreateViewContext* ctx) override;

    SoundRecorderBody& body() { return m_body; }

private:
    SoundRecorderBody m_body;
};

} // namespace os

#endif

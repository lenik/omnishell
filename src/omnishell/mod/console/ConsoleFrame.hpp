#ifndef OMNISHELL_MOD_CONSOLE_FRAME_HPP
#define OMNISHELL_MOD_CONSOLE_FRAME_HPP

#include "ConsoleBody.hpp"

#include <bas/wx/uiframe.hpp>

namespace os {

class App;

class ConsoleFrame : public uiFrame {
public:
    explicit ConsoleFrame(App* app);

    void createFragmentView(CreateViewContext* ctx) override;

private:
    ConsoleBody m_body;
};

} // namespace os

#endif

#ifndef OMNISHELL_MOD_CONSOLE_FRAME_HPP
#define OMNISHELL_MOD_CONSOLE_FRAME_HPP

#include "ConsoleBody.hpp"

#include <wx/frame.h>

namespace os {

class App;

class ConsoleFrame : public wxFrame {
public:
    explicit ConsoleFrame(App* app);

    ConsoleBody* body() { return m_body; }

private:
    ConsoleBody* m_body{nullptr};
};

} // namespace os

#endif

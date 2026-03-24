#ifndef OMNISHELL_MOD_CONSOLE_BODY_HPP
#define OMNISHELL_MOD_CONSOLE_BODY_HPP

#include "../../wx/wxConsole.hpp"

#include <wx/window.h>

namespace os {

/** Application-specific zash builtins (example: hello). */
class ConsoleBody : public wxConsole {
public:
    ConsoleBody(wxWindow* parent, wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                long style = 0);

protected:
    void OnZashInit(zash::Interpreter& interp) override;
};

} // namespace os

#endif

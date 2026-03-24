#include "ConsoleBody.hpp"

#include "cli/zash/zash_interpreter.hpp"

namespace os {

namespace {

int bi_hello(int, char**, zash::Interpreter& interp) {
    if (interp.writeOut)
        interp.writeOut("Hello from the Console module.\r\n");
    return 0;
}

} // namespace

ConsoleBody::ConsoleBody(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                         long style)
    : wxConsole(parent, id, pos, size, style) {
    if (GetTerminal())
        GetTerminal()->WriteUtf8("Tip: try the `hello` command.\r\n");
}

void ConsoleBody::OnZashInit(zash::Interpreter& interp) {
    interp.registerBuiltin("hello", bi_hello);
    wxConsole::OnZashInit(interp);
}

} // namespace os

#ifndef OMNISHELL_WX_TERM_XTERM_HPP
#define OMNISHELL_WX_TERM_XTERM_HPP

#include "TermInterpreter.hpp"

#include <cstddef>
#include <vector>

namespace os {

class wxTerminal;

namespace term {

/** Decodes ESC / CSI (xterm-like) and drives wxTerminal Set* / cursor / erase APIs. */
class XtermInterpreter : public TermInterpreter {
public:
    explicit XtermInterpreter(wxTerminal& term);

    bool Receive(uint32_t codePoint) override;
    void Reset() override;

private:
    enum class EscState { Ground, Esc, Csi, CsiQuest };

    bool EvalCsi(char finalCh, bool question);
    static int ParamAt(const std::vector<int>& params, size_t i, int defVal);

    wxTerminal& m_term;
    EscState m_esc{EscState::Ground};
    std::vector<int> m_csiParams;
    int m_csiParamCur{0};
};

} // namespace term

} // namespace os

#endif

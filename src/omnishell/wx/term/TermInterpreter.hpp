#ifndef OMNISHELL_WX_TERM_TERM_INTERPRETER_HPP
#define OMNISHELL_WX_TERM_TERM_INTERPRETER_HPP

#include <cstdint>

namespace os {

/**
 * Pluggable filter on the output stream: full Unicode code points (UTF-8 already decoded).
 * Return true if the code point was consumed; false to try the next interpreter or PutChar.
 */
class TermInterpreter {
public:
    virtual ~TermInterpreter() = default;
    virtual bool Receive(uint32_t codePoint) = 0;
    virtual void Reset() {}
};

} // namespace os

#endif

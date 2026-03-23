#ifndef OMNISHELL_WX_SAFE_WX_HANDLER_HPP
#define OMNISHELL_WX_SAFE_WX_HANDLER_HPP

#include "WxChecked.hpp"

#include <utility>

namespace os {

/** For EVT_* handlers with no wxEvent* handy, or non-window handlers (e.g. tray): inferred as handler · callback. */
template <typename F>
void wxSafeCall(F&& f) {
    wxInvokeChecked(nullptr, nullptr, std::forward<F>(f));
}

} // namespace os

#endif

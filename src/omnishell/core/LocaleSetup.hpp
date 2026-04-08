#ifndef OMNISHELL_CORE_LOCALE_SETUP_HPP
#define OMNISHELL_CORE_LOCALE_SETUP_HPP

#include <wx/intl.h>

namespace os {

/** Initialize wxWidgets locale and load the omnishell message catalog. Keeps @a locale alive. */
bool setupOmniShellLocale(wxLocale& locale);

} // namespace os

#endif

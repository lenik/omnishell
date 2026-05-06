#ifndef OMNISHELL_MOD_CONSOLE_BODY_HPP
#define OMNISHELL_MOD_CONSOLE_BODY_HPP

#include "../../core/App.hpp"
#include "../../wx/wxConsole.hpp"

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/window.h>

namespace os {

/** Application-specific zash builtins (example: hello). */
class ConsoleBody : public UIFragment {
  public:
    ConsoleBody(App* app);
    ~ConsoleBody() override;

    void createFragmentView(CreateViewContext* ctx) override;

    wxConsole* GetConsole() const { return m_console; }
    wxTerminal* GetTerminal() const { return m_console ? m_console->GetTerminal() : nullptr; }

  private:
    void loadHistory();
    void saveHistory();

    void onFrameClose(wxCloseEvent& e);

  private:
    App* m_app{nullptr};
    uiFrame* m_frame{nullptr};
    wxConsole* m_console{nullptr};
};

} // namespace os

#endif

#ifndef OMNISHELL_WX_WX_CONSOLE_HPP
#define OMNISHELL_WX_WX_CONSOLE_HPP

#include "wxTerminal.hpp"

#include <wx/panel.h>

#include <memory>
#include <mutex>

namespace os {

namespace zash {
class Interpreter;
}

/**
 * Shell on top of wxTerminal using the zash parser and interpreter (bash-like).
 * Subclass and override OnZashInit to register extra builtins (see mod/console/ConsoleBody).
 */
class wxConsole : public wxPanel {
public:
    wxConsole(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize, long style = 0);
    ~wxConsole() override;

    wxTerminal* GetTerminal() const { return m_term; }
    zash::Interpreter* GetZash() const { return m_zash.get(); }

protected:
    /**
     * Called from the wxConsole constructor after m_zash is created.
     * Note: C++ does not call derived overrides during base construction — register extra builtins
     * in the derived constructor body (see ConsoleBody) or accept duplicate registration in OnZashInit
     * for any later explicit init path.
     */
    virtual void OnZashInit(zash::Interpreter& interp);

private:
    void OnSubmit(const wxString& line);
    void SyncPromptFromZash();
    bool TryTabComplete(wxString& edit, size_t& caret, wxTerminal* term);

    wxTerminal* m_term{nullptr};
    std::unique_ptr<zash::Interpreter> m_zash;
    /** Serializes zash execution vs tab-completion / prompt reads. */
    std::mutex m_zashMu;
    wxString m_tabSig;
};

} // namespace os

#endif

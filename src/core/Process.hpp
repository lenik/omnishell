#ifndef OMNISHELL_CORE_PROCESS_HPP
#define OMNISHELL_CORE_PROCESS_HPP

#include <bas/ui/arch/ImageSet.hpp>

#include <wx/window.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace os {

class Process;
using ProcessPtr = std::shared_ptr<Process>;

/**
 * Process
 *
 * Runtime instance created by a Module::run().
 * A module may create multiple windows per run; all should be registered here.
 */
class Process : public std::enable_shared_from_this<Process> {
public:
    std::string id;        // unique-ish id for this process instance
    std::string uri;       // module uri (best effort)
    std::string name;      // module name (best effort)
    std::string label;     // display label (for taskbar)
    ImageSet icon;         // icon set (for taskbar)

    Process();

    void addWindow(wxWindow* w);
    void setOnWindowAdded(std::function<void(ProcessPtr, wxWindow*)> cb) { m_onWindowAdded = std::move(cb); }
    const std::vector<wxWindow*>& windows() const { return m_windows; }
    wxWindow* primaryWindow() const { return m_windows.empty() ? nullptr : m_windows.front(); }

private:
    std::vector<wxWindow*> m_windows;
    std::function<void(ProcessPtr, wxWindow*)> m_onWindowAdded;
};

} // namespace os

#endif // OMNISHELL_CORE_PROCESS_HPP


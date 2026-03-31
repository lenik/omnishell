#include "ConsoleBody.hpp"

#include "../../core/App.hpp"
#include "../../core/registry/RegistryService.hpp"
#include "../../ui/ThemeStyles.hpp"
#include "../../wx/artprovs.hpp"
#include "../../wx/wxConsole.hpp"

#include <wx/panel.h>
#include <wx/sizer.h>

#include <algorithm>
#include <sstream>

#include <zash_interpreter.hpp>

using namespace ThemeStyles;

namespace os {

namespace {

int bi_hello(int, char**, zash::Interpreter& interp) {
    if (interp.writeOut)
        interp.writeOut("Hello from the Console module.\r\n");
    return 0;
}

constexpr const char* kConsoleHistoryKey = "OmniShell.Console.history";
constexpr size_t kMaxHistoryLines = 300;

static std::vector<std::string> splitLines(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == '\n') {
            out.push_back(cur);
            cur.clear();
        } else if (c != '\r') {
            cur.push_back(c);
        }
    }
    if (!cur.empty())
        out.push_back(cur);
    return out;
}

static std::string joinLines(const std::vector<std::string>& v) {
    std::ostringstream oss;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i)
            oss << "\n";
        oss << v[i];
    }
    return oss.str();
}

} // namespace

ConsoleBody::ConsoleBody(App* app) : m_app(app) {
    const IconTheme* theme = app->getIconTheme();

    // Use a dedicated group id range for console actions.
    group(120050, "system", "console", 1200)
        .label("&Console")
        .description("Console")
        .icon(theme->icon("console", "group.console"))
        .install();

    int seq = 0;
    action(120051, "system/console", "clear", seq++)
        .label("&Clear")
        .description("Clear screen")
        .icon(theme->icon("console", "edit.clear"))
        .performFn([this](PerformContext*) {
            if (m_console) {
                if (wxTerminal* t = m_console->GetTerminal()) {
                    t->ClearScreen();
                    t->SetFocus();
                }
            }
        })
        .install();

    action(120052, "system/console", "font_up", seq++)
        .label("Font &+")
        .description("Increase font size")
        .icon(theme->icon("console", "view.zoom_in"))
        .performFn([this](PerformContext*) {
            if (m_console) {
                if (wxTerminal* t = m_console->GetTerminal()) {
                    t->FontSizeUp(1);
                    t->SetFocus();
                }
            }
        })
        .install();

    action(120053, "system/console", "font_down", seq++)
        .label("Font &-")
        .description("Decrease font size")
        .icon(theme->icon("console", "view.zoom_in"))
        .performFn([this](PerformContext*) {
            if (m_console) {
                if (wxTerminal* t = m_console->GetTerminal()) {
                    t->FontSizeDown(1);
                    t->SetFocus();
                }
            }
        })
        .install();

    action(120054, "system/console", "font_reset", seq++)
        .label("Font &Reset")
        .description("Reset font size")
        .icon(theme->icon("console", "view.zoom_reset"))
        .performFn([this](PerformContext*) {
            if (m_console) {
                if (wxTerminal* t = m_console->GetTerminal()) {
                    t->FontSizeReset();
                    t->SetFocus();
                }
            }
        })
        .install();

    action(120055, "system/console", "cmd_prev", seq++)
        .label("&Previous Command")
        .description("History previous")
        .icon(theme->icon("console", "nav.history_prev"))
        .performFn([this](PerformContext*) {
            if (m_console) {
                if (wxTerminal* t = m_console->GetTerminal()) {
                    t->HistoryPrev();
                    t->SetFocus();
                }
            }
        })
        .install();

    action(120056, "system/console", "cmd_next", seq++)
        .label("&Next Command")
        .description("History next")
        .icon(theme->icon("console", "nav.history_next"))
        .performFn([this](PerformContext*) {
            if (m_console) {
                if (wxTerminal* t = m_console->GetTerminal()) {
                    t->HistoryNext();
                    t->SetFocus();
                }
            }
        })
        .install();

    action(120057, "system/console", "page_prev", seq++)
        .label("Previous &Page")
        .description("Scroll up one page")
        .icon(theme->icon("console", "nav.page_prev"))
        .performFn([this](PerformContext*) {
            if (m_console) {
                if (wxTerminal* t = m_console->GetTerminal()) {
                    t->PagePrev();
                    t->SetFocus();
                }
            }
        })
        .install();

    action(120058, "system/console", "page_next", seq++)
        .label("Next P&age")
        .description("Scroll down one page")
        .icon(theme->icon("console", "nav.page_next"))
        .performFn([this](PerformContext*) {
            if (m_console) {
                if (wxTerminal* t = m_console->GetTerminal()) {
                    t->PageNext();
                    t->SetFocus();
                }
            }
        })
        .install();
}

ConsoleBody::~ConsoleBody() = default;

wxEvtHandler* ConsoleBody::getEventHandler() {
    if (m_console) {
        if (wxTerminal* t = m_console->GetTerminal())
            return t->GetEventHandler();
        return m_console->GetEventHandler();
    }
    return m_frame ? m_frame->GetEventHandler() : nullptr;
}

void ConsoleBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    m_frame = dynamic_cast<uiFrame*>(parent);
    if (!m_frame)
        return;

    m_console = new wxConsole(m_frame);

    auto* sz = new wxBoxSizer(wxVERTICAL);
    sz->Add(m_console, 1, wxEXPAND);
    m_frame->SetSizer(sz);

    loadHistory();

    m_frame->Bind(wxEVT_CLOSE_WINDOW, &ConsoleBody::onFrameClose, this);
}

void ConsoleBody::onFrameClose(wxCloseEvent& e) {
    saveHistory();
    e.Skip();
}

void ConsoleBody::loadHistory() {
    if (!m_console)
        return;
    wxTerminal* t = m_console->GetTerminal();
    if (!t)
        return;

    IRegistry& r = registry();
    if (!r.has(kConsoleHistoryKey))
        return;
    const std::string raw = r.getString(kConsoleHistoryKey, "");
    if (raw.empty())
        return;
    auto lines = splitLines(raw);
    if (lines.size() > kMaxHistoryLines)
        lines.erase(lines.begin(), lines.end() - kMaxHistoryLines);

    t->SetCommandHistoryUtf8(lines);
}

void ConsoleBody::saveHistory() {
    if (!m_console)
        return;
    wxTerminal* t = m_console->GetTerminal();
    if (!t)
        return;

    auto lines = t->GetCommandHistoryUtf8();
    if (lines.size() > kMaxHistoryLines)
        lines.erase(lines.begin(), lines.end() - kMaxHistoryLines);

    IRegistry& r = registry();
    r.set(kConsoleHistoryKey, joinLines(lines));
    r.save();
}

} // namespace os

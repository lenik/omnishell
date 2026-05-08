#include "ConsoleBody.hpp"

#include "../../core/App.hpp"
#include "../../core/registry/RegistryService.hpp"
#include "../../ui/ThemeStyles.hpp"
#include "../../wx/artprovs.hpp"
#include "../../wx/wxConsole.hpp"

#include <zash/zash_interpreter.hpp>

#include <wx/panel.h>
#include <wx/sizer.h>

#include <algorithm>
#include <sstream>

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

enum {
    ID_GROUP_CONSOLE = uiFrame::ID_APP_HIGHEST + 1,
    ID_CLEAR,
    ID_FONT_UP,
    ID_FONT_DOWN,
    ID_FONT_RESET,
    ID_CMD_PREV,
    ID_CMD_NEXT,
    ID_PAGE_PREV,
    ID_PAGE_NEXT,
};

ConsoleBody::ConsoleBody(App* app) : m_app(app) {
    const IconTheme* theme = app->getIconTheme();

    // Use a dedicated group id range for console actions.
    group(ID_GROUP_CONSOLE, "system", "console", 1200)
        .label("&Console")
        .description("Console")
        .icon(theme->icon("console", "group.console"))
        .install();

    int seq = 0;
    action(ID_CLEAR, "system/console", "clear", seq++)
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

    action(ID_FONT_UP, "system/console", "font_up", seq++)
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

    action(ID_FONT_DOWN, "system/console", "font_down", seq++)
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

    action(ID_FONT_RESET, "system/console", "font_reset", seq++)
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

    action(ID_CMD_PREV, "system/console", "cmd_prev", seq++)
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

    action(ID_CMD_NEXT, "system/console", "cmd_next", seq++)
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

    action(ID_PAGE_PREV, "system/console", "page_prev", seq++)
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

    action(ID_PAGE_NEXT, "system/console", "page_next", seq++)
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

wxWindow* ConsoleBody::createFragmentView(CreateViewContext* ctx) {
    m_frame = ctx->findParentFrame();

    wxWindow* parent = ctx->getParent();
    m_console = new wxConsole(parent);
    m_frame->Bind(wxEVT_CLOSE_WINDOW, &ConsoleBody::onFrameClose, this);

    loadHistory();

    return m_console;
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

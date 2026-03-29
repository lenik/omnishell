#include "wxConsole.hpp"

#include "../cli/zash/parser/zash_ast.h"
#include "../cli/zash/zash_driver.hpp"
#include "../cli/zash/zash_interpreter.hpp"

#include <wx/utils.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace os {

namespace {

wxString trimWx(const wxString& s) {
    wxString t = s;
    t.Trim(false);
    t.Trim(true);
    return t;
}

int wx_zash_term_clear(int, char**, zash::Interpreter& ip) {
    auto* t = static_cast<wxTerminal*>(ip.userData);
    if (t)
        t->ClearScreen();
    return 0;
}

std::string wxToUtf8(const wxString& s) {
    wxScopedCharBuffer b = s.utf8_str();
    const char* p = b.data();
    return p ? std::string(p, b.length()) : std::string();
}

wxString utf8ToWx(std::string_view sv) {
    if (sv.empty())
        return wxString();
    return wxString::FromUTF8(sv.data(), static_cast<int>(sv.size()));
}

size_t wordStartAt(const wxString& edit, size_t caret) {
    if (caret > edit.length())
        caret = edit.length();
    for (size_t i = caret; i > 0;) {
        --i;
        const wxChar c = edit[i];
        if (c == wxT(' ') || c == wxT('\t')) {
            return i + 1;
        }
    }
    return 0;
}

bool onlyWhitespaceBefore(const wxString& edit, size_t wordStart) {
    for (size_t i = 0; i < wordStart; ++i) {
        const wxChar c = edit[i];
        if (c != wxT(' ') && c != wxT('\t'))
            return false;
    }
    return true;
}

std::string longestCommonPrefix(const std::vector<std::string>& v) {
    if (v.empty())
        return {};
    std::string p = v[0];
    for (size_t i = 1; i < v.size(); ++i) {
        const std::string& s = v[i];
        size_t n = 0;
        const size_t lim = std::min(p.size(), s.size());
        while (n < lim && p[n] == s[n])
            ++n;
        p.resize(n);
        if (p.empty())
            break;
    }
    return p;
}

void collectPathCommands(const std::string& prefix, std::set<std::string>& out) {
#if defined(__unix__) || defined(__APPLE__)
    if (prefix.empty())
        return;
    namespace fs = std::filesystem;
    const char* pathEnv = getenv("PATH");
    if (!pathEnv)
        return;
    std::string_view rest(pathEnv);
    while (!rest.empty()) {
        const size_t col = rest.find(':');
        const std::string_view dirSv =
            (col == std::string_view::npos) ? rest : rest.substr(0, col);
        rest = (col == std::string_view::npos) ? std::string_view{} : rest.substr(col + 1);
        if (dirSv.empty())
            continue;
        try {
            const fs::path dirPath{std::string(dirSv)};
            if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
                continue;
            for (const fs::directory_entry& ent : fs::directory_iterator(dirPath)) {
                std::error_code ec;
                const auto st = ent.symlink_status(ec);
                if (ec)
                    continue;
                if (!fs::is_regular_file(st) && !fs::is_symlink(st))
                    continue;
                std::string name = ent.path().filename().string();
                if (name.size() < prefix.size() || name.compare(0, prefix.size(), prefix) != 0)
                    continue;
                if (access(ent.path().string().c_str(), X_OK) != 0)
                    continue;
                out.insert(std::move(name));
            }
        } catch (...) {
        }
    }
#else
    (void)prefix;
    (void)out;
#endif
}

std::filesystem::path resolveDirPath(const std::string& relDir) {
    namespace fs = std::filesystem;
    if (relDir.empty())
        return fs::current_path();
    fs::path p(relDir);
    if (p.is_absolute())
        return p.lexically_normal();
    return (fs::current_path() / p).lexically_normal();
}

void collectFileMatches(const std::string& token, bool showDotfiles, std::vector<std::string>& out) {
    namespace fs = std::filesystem;
    std::string relDir;
    std::string namePrefix;
    if (token == "/") {
        relDir = "/";
    } else if (!token.empty() && token.back() == '/') {
        relDir = token.substr(0, token.size() - 1);
        namePrefix.clear();
    } else {
        const size_t slash = token.rfind('/');
        if (slash == std::string::npos) {
            relDir.clear();
            namePrefix = token;
        } else {
            relDir = token.substr(0, slash);
            namePrefix = token.substr(slash + 1);
        }
    }

    const fs::path dirPath = resolveDirPath(relDir);
    try {
        if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
            return;
        for (const fs::directory_entry& ent : fs::directory_iterator(dirPath)) {
            std::error_code ec;
            const std::string fname = ent.path().filename().string();
            if (fname.empty())
                continue;
            if (!showDotfiles && !namePrefix.empty() && namePrefix[0] != '.' && fname[0] == '.')
                continue;
            if (fname.size() < namePrefix.size() ||
                fname.compare(0, namePrefix.size(), namePrefix) != 0)
                continue;
            const bool isDir = ent.is_directory(ec);
            if (ec)
                continue;
            std::string repl = relDir.empty() ? fname : (relDir + "/" + fname);
            if (isDir)
                repl += '/';
            out.push_back(std::move(repl));
        }
    } catch (...) {
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
}

void writeMatchList(wxTerminal* term, const std::vector<std::string>& matches) {
    if (!term || matches.empty())
        return;
    std::string acc;
    for (const auto& s : matches) {
        acc += s;
        acc += "  ";
    }
    acc += "\r\n";
    term->WriteUtf8BelowInputOverlay(acc);
}

} // namespace

void wxConsole::OnZashInit(zash::Interpreter&) {}

bool wxConsole::TryTabComplete(wxString& edit, size_t& caret, wxTerminal* term) {
    if (!term || !m_zash)
        return false;
    std::lock_guard<std::mutex> lock(m_zashMu);

    const size_t wordStart = wordStartAt(edit, caret);
    const wxString tokenW = edit.Mid(wordStart, caret - wordStart);
    const std::string token = wxToUtf8(tokenW);
    const bool firstWord = onlyWhitespaceBefore(edit, wordStart);

    std::vector<std::string> matches;

    if (firstWord) {
        if (token.empty()) {
            m_zash->builtinNames(matches);
            std::vector<std::string> fns;
            m_zash->functionNames(fns);
            matches.insert(matches.end(), fns.begin(), fns.end());
            fns.clear();
            m_zash->aliasNames(fns);
            matches.insert(matches.end(), fns.begin(), fns.end());
            std::sort(matches.begin(), matches.end());
            matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
        } else {
            std::set<std::string> cmdSet;
            std::vector<std::string> tmp;
            m_zash->builtinNames(tmp);
            for (const auto& s : tmp) {
                if (s.size() >= token.size() && s.compare(0, token.size(), token) == 0)
                    cmdSet.insert(s);
            }
            tmp.clear();
            m_zash->functionNames(tmp);
            for (const auto& s : tmp) {
                if (s.size() >= token.size() && s.compare(0, token.size(), token) == 0)
                    cmdSet.insert(s);
            }
            tmp.clear();
            m_zash->aliasNames(tmp);
            for (const auto& s : tmp) {
                if (s.size() >= token.size() && s.compare(0, token.size(), token) == 0)
                    cmdSet.insert(s);
            }
            collectPathCommands(token, cmdSet);
            matches.assign(cmdSet.begin(), cmdSet.end());
        }
    } else {
        const bool showDot = !token.empty() && token[0] == '.';
        collectFileMatches(token, showDot, matches);
    }

    if (matches.empty()) {
        wxBell();
        m_tabSig.clear();
        return true;
    }

    const std::string lcp = longestCommonPrefix(matches);
    if (lcp.size() > token.size()) {
        const wxString rep = utf8ToWx(lcp);
        edit.erase(wordStart, caret - wordStart);
        edit.insert(wordStart, rep);
        caret = wordStart + rep.length();
        m_tabSig.clear();
        return true;
    }

    if (matches.size() == 1 && matches[0] == token) {
        wxBell();
        return true;
    }

    if (matches.size() == 1) {
        const wxString rep = utf8ToWx(matches[0]);
        edit.erase(wordStart, caret - wordStart);
        edit.insert(wordStart, rep);
        caret = wordStart + rep.length();
        m_tabSig.clear();
        return true;
    }

    wxString sig;
    sig << wxString::Format(wxS("%zu"), wordStart) << wxChar('\x1f') << tokenW << wxChar('\x1f')
        << edit.Left(caret);
    if (sig == m_tabSig) {
        writeMatchList(term, matches);
        m_tabSig.clear();
    } else {
        wxBell();
        m_tabSig = sig;
    }
    return true;
}

wxConsole::~wxConsole() = default;

void wxConsole::SyncPromptFromZash() {
    if (!m_term || !m_zash)
        return;
    std::string raw = m_zash->getVar("PS1");
    if (raw.empty())
        raw = "> ";
    const std::string exp = m_zash->expandPS1(raw);
    m_term->SetPromptUtf8(exp);
    m_term->Refresh();
}

wxConsole::wxConsole(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                     long style)
    : wxPanel(parent, id, pos, size, style) {
    SetName(wxT("wxConsole"));
    m_term = new wxTerminal(this, wxID_ANY);

    wxBoxSizer* sz = new wxBoxSizer(wxVERTICAL);
    sz->Add(m_term, 1, wxEXPAND);
    SetSizer(sz);

    m_term->OnSubmitLine = [this](const wxString& line) { OnSubmit(line); };
    m_term->OnTabComplete = [this](wxString& edit, size_t& caret, wxTerminal* t) {
        return TryTabComplete(edit, caret, t);
    };
    m_term->OnEofEmptyLine = [this]() {
        wxWindow* tl = wxGetTopLevelParent(this);
        if (tl)
            tl->Close(false);
    };

    m_zash = std::make_unique<zash::Interpreter>();
    m_zash->setVar("TERM", "xterm-256color");
    /* Embedded terminal: avoid interactive pagers (less) blocking read(pty) forever on UI thread. */
    m_zash->setVar("PAGER", "cat");
    m_zash->setVar("MANPAGER", "cat");
    m_zash->setVar("GIT_PAGER", "cat");
    m_zash->userData = m_term;
    auto postOut = [this](std::string_view sv) {
        std::string chunk(sv);
        wxTerminal* t = m_term;
        t->CallAfter([t, chunk = std::move(chunk)]() { t->WriteUtf8(chunk); });
    };
    m_zash->writeOut = postOut;
    m_zash->writeErr = postOut;
    m_zash->requestExit = [this]() {
        wxWindow* tl = wxGetTopLevelParent(this);
        if (tl)
            tl->CallAfter([tl]() { tl->Close(false); });
    };
    m_zash->registerBuiltin("clear", wx_zash_term_clear);
    m_zash->registerBuiltin("cls", wx_zash_term_clear);

    m_term->OnSendForegroundPty = [this](std::string_view sv) {
        if (m_zash)
            m_zash->queueForegroundPtyInput(sv);
    };
    m_zash->onForegroundPty = [this](bool on) {
        wxTerminal* t = m_term;
        t->CallAfter([t, on]() { t->SetForegroundPtyPassthrough(on); });
    };

    OnZashInit(*m_zash);
    SyncPromptFromZash();

    /* After layout, client width is non-zero so RecalcGrid() has set m_cols > kMinCols (20). */
    m_term->CallAfter([t = m_term]() {
        if (t)
            t->WriteUtf8(
                "OmniShell console — zash (if/for/case, pipelines, builtins). Type `help`.\r\n");
    });
}

void wxConsole::OnSubmit(const wxString& line) {
    wxString t = trimWx(line);
    if (t.empty())
        return;

    wxScopedCharBuffer utf8 = t.utf8_str();
    const char* p = utf8.data();
    if (!p)
        return;

    ZashProgram* prog = nullptr;
    int pr = zash_parse_line(p, &prog);
    if (pr != 0) {
        if (prog)
            zash_free_program(prog);
        if (m_zash->writeErr) {
            const char* detail = zash_parse_last_error();
            std::string msg = "zash: ";
            if (pr < 0)
                msg += "out of memory";
            else if (detail && detail[0])
                msg += detail;
            else
                msg += "syntax error";
            msg += "\r\n";
            m_zash->writeErr(msg);
        }
        return;
    }

    std::thread([this, prog]() {
        {
            std::lock_guard<std::mutex> lock(m_zashMu);
            m_zash->runProgram(prog);
        }
        wxTerminal* term = m_term;
        term->CallAfter([this, term]() {
            SyncPromptFromZash();
            term->Refresh();
        });
    }).detach();
}

} // namespace os

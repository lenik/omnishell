#include "wxTerminal.hpp"

#include "term/xterm.hpp"

#include <wx/clipbrd.h>
#include <wx/dcbuffer.h>

#include <algorithm>
#include <climits>
#include <cwchar>
#include <functional>
#include <string_view>
#include <vector>

namespace os {

namespace {

constexpr int kMinCols = 20;
constexpr int kMinRows = 4;
constexpr int kScrollbackExtraRows = 2048;

// wxGTK: wxScrollBar may still grab focus even if SetCanFocus(false) is used.
// Enforce non-focusable behavior by overriding AcceptsFocus.
class NoFocusScrollBar final : public wxScrollBar {
public:
    NoFocusScrollBar(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
        : wxScrollBar(parent, id, pos, size, style) {}

    bool AcceptsFocus() const override { return false; }
    bool AcceptsFocusFromKeyboard() const override { return false; }
};

void Utf8Consume(std::string_view& in, uint32_t& outCp, bool& ok) {
    ok = false;
    if (in.empty())
        return;
    unsigned char b0 = static_cast<unsigned char>(in[0]);
    if (b0 < 0x80u) {
        outCp = b0;
        in.remove_prefix(1);
        ok = true;
        return;
    }
    auto need = 0;
    uint32_t cp = 0;
    if ((b0 & 0xe0u) == 0xc0u) {
        need = 2;
        cp = b0 & 0x1fu;
    } else if ((b0 & 0xf0u) == 0xe0u) {
        need = 3;
        cp = b0 & 0x0fu;
    } else if ((b0 & 0xf8u) == 0xf0u) {
        need = 4;
        cp = b0 & 0x07u;
    } else {
        outCp = 0xfffd;
        in.remove_prefix(1);
        ok = true;
        return;
    }
    if (static_cast<int>(in.size()) < need) {
        outCp = 0xfffd;
        in = {};
        ok = true;
        return;
    }
    for (int i = 1; i < need; ++i) {
        unsigned char b = static_cast<unsigned char>(in[static_cast<size_t>(i)]);
        if ((b & 0xc0u) != 0x80u) {
            outCp = 0xfffd;
            in.remove_prefix(1);
            ok = true;
            return;
        }
        cp = (cp << 6) | (b & 0x3fu);
    }
    in.remove_prefix(static_cast<size_t>(need));
    outCp = cp;
    ok = true;
}

void ForEachUtf8Cp(std::string_view utf8, const std::function<void(uint32_t)>& fn) {
    std::string_view rest = utf8;
    while (!rest.empty()) {
        uint32_t cp = 0;
        bool ok = false;
        Utf8Consume(rest, cp, ok);
        if (ok)
            fn(cp);
    }
}

wxColour BlendTowards(const wxColour& base, const wxColour& tint, unsigned amount) {
    amount = std::min(255u, amount);
    const auto mix = [&](unsigned a, unsigned b) -> unsigned char {
        return static_cast<unsigned char>((a * (255u - amount) + b * amount) / 255u);
    };
    return wxColour(mix(base.Red(), tint.Red()), mix(base.Green(), tint.Green()),
                    mix(base.Blue(), tint.Blue()));
}

void RTrimLine(wxString& s) {
    while (!s.empty()) {
        const wxChar last = s.Last();
        if (last == wxT(' ') || last == wxT('\t') || last == wxT('\r') || last == wxT('\n'))
            s.RemoveLast();
        else
            break;
    }
}

const wxColour kSelTint(173, 214, 255);

struct PromptGfx {
    wxColour fg{0, 0, 0};
    wxColour bg{255, 255, 255};
    bool bold{false};
    bool italic{false};
    bool inverse{false};
};

static wxColour PromptAnsiFgStd(int code) {
    static const wxColour tbl[8] = {wxColour(0, 0, 0),       wxColour(205, 49, 49),
                                    wxColour(13, 179, 0),    wxColour(229, 193, 0),
                                    wxColour(36, 114, 200),  wxColour(188, 63, 188),
                                    wxColour(17, 168, 171),  wxColour(229, 229, 229)};
    if (code >= 0 && code <= 7)
        return tbl[code];
    return wxColour(0, 0, 0);
}

static wxColour PromptAnsiFgBright(int code) {
    static const wxColour tbl[8] = {
        wxColour(80, 80, 80),    wxColour(255, 100, 100), wxColour(100, 255, 100), wxColour(255, 255, 100),
        wxColour(100, 160, 255), wxColour(255, 120, 255), wxColour(100, 240, 240), wxColour(255, 255, 255)};
    if (code >= 0 && code <= 7)
        return tbl[code];
    return wxColour(255, 255, 255);
}

static wxColour PromptAnsiBgStd(int code) {
    return PromptAnsiFgStd(code);
}

static std::vector<int> ParseCsiSemis(std::string_view mid) {
    std::vector<int> a;
    int cur = -1;
    for (char ch : mid) {
        const auto uc = static_cast<unsigned char>(ch);
        if (uc >= '0' && uc <= '9') {
            const int d = static_cast<int>(uc - '0');
            cur = (cur < 0) ? d : (cur * 10 + d);
        } else if (ch == ';') {
            a.push_back(cur < 0 ? 0 : cur);
            cur = -1;
        }
    }
    a.push_back(cur < 0 ? 0 : cur);
    return a;
}

static void ApplyPromptSgr(const std::vector<int>& args, PromptGfx& st, const wxColour& defFg,
                           const wxColour& defBg) {
    if (args.empty()) {
        st.fg = defFg;
        st.bg = defBg;
        st.bold = st.italic = st.inverse = false;
        return;
    }
    for (size_t i = 0; i < args.size(); ++i) {
        int v = args[i];
        if (v == 0) {
            st.fg = defFg;
            st.bg = defBg;
            st.bold = st.italic = st.inverse = false;
        } else if (v == 1) {
            st.bold = true;
        } else if (v == 3) {
            st.italic = true;
        } else if (v == 7) {
            st.inverse = true;
        } else if (v == 22) {
            st.bold = false;
        } else if (v == 23) {
            st.italic = false;
        } else if (v == 27) {
            st.inverse = false;
        } else if (v >= 30 && v <= 37) {
            st.fg = PromptAnsiFgStd(v - 30);
        } else if (v >= 90 && v <= 97) {
            st.fg = PromptAnsiFgBright(v - 90);
        } else if (v == 39) {
            st.fg = defFg;
        } else if (v >= 40 && v <= 47) {
            st.bg = PromptAnsiBgStd(v - 40);
        } else if (v >= 100 && v <= 107) {
            st.bg = PromptAnsiFgBright(v - 100);
        } else if (v == 49) {
            st.bg = defBg;
        } else if (v == 38 && i + 4 < args.size() && args[i + 1] == 2) {
            st.fg = wxColour(args[i + 2], args[i + 3], args[i + 4]);
            i += 4;
        } else if (v == 48 && i + 4 < args.size() && args[i + 1] == 2) {
            st.bg = wxColour(args[i + 2], args[i + 3], args[i + 4]);
            i += 4;
        }
    }
}

static size_t ConsumeOsc(std::string_view p, size_t i) {
    /* i = index of ']' after ESC */
    size_t j = i + 1;
    while (j < p.size()) {
        if (p[j] == '\a')
            return j + 1;
        if (p[j] == '\033' && j + 1 < p.size() && p[j + 1] == '\\')
            return j + 2;
        ++j;
    }
    return p.size();
}

static size_t ConsumeCsi(std::string_view p, size_t i, PromptGfx& st, const wxColour& defFg,
                         const wxColour& defBg) {
    /* i = index of '[' after ESC */
    size_t j = i + 1;
    while (j < p.size()) {
        const auto cj = static_cast<unsigned char>(p[j]);
        if (cj >= 0x40u && cj <= 0x7eu) {
            const char fin = static_cast<char>(cj);
            if (fin == 'm') {
                const std::string_view mid(p.data() + i + 1, j - (i + 1));
                ApplyPromptSgr(ParseCsiSemis(mid), st, defFg, defBg);
            }
            return j + 1;
        }
        ++j;
    }
    return p.size();
}

static size_t ConsumeEsc(std::string_view p, size_t i, PromptGfx& st, const wxColour& defFg,
                         const wxColour& defBg) {
    if (i + 1 >= p.size())
        return p.size();
    const auto c1 = static_cast<unsigned char>(p[i + 1]);
    if (c1 == '[')
        return ConsumeCsi(p, i + 1, st, defFg, defBg);
    if (c1 == ']')
        return ConsumeOsc(p, i + 1);
    return i + 2;
}

static bool CodepointLikelyWide(uint32_t cp) {
    if (cp >= 0x2e80u && cp <= 0x9fffu)
        return true;
    if (cp >= 0xf900u && cp <= 0xfaffu)
        return true;
    if (cp >= 0xfe10u && cp <= 0xfe6fu)
        return true;
    if (cp >= 0xff00u && cp <= 0xff60u)
        return true;
    if (cp >= 0xffe0u && cp <= 0xffe6u)
        return true;
    if (cp >= 0x1f300u && cp <= 0x1faffu)
        return true;
    if (cp >= 0x20000u && cp <= 0x2fffdu)
        return true;
    if (cp >= 0x30000u && cp <= 0x3fffdu)
        return true;
    return false;
}

/** Terminal column width for one Unicode scalar value (1 or 2 for this grid). */
static int CodepointColumnWidth(uint32_t cp) {
    if (cp < 32u && cp != '\t')
        return 0;
    if (cp >= 0x110000u)
        return 1;
#if defined(__unix__) || defined(__APPLE__)
    int n = -1;
#if WCHAR_MAX >= 0x10ffff
    {
        const wchar_t w = static_cast<wchar_t>(cp);
        if (static_cast<uint32_t>(w) == cp)
            n = wcwidth(w);
    }
#else
    if (cp <= static_cast<uint32_t>(WCHAR_MAX))
        n = wcwidth(static_cast<wchar_t>(cp));
#endif
    if (n < 0)
        return CodepointLikelyWide(cp) ? 2 : 1;
    if (n == 0)
        return 1;
    return n > 2 ? 2 : n;
#else
    return CodepointLikelyWide(cp) ? 2 : 1;
#endif
}

static void ParsePromptUtf8(std::string_view p, const wxColour& defFg, const wxColour& defBg,
                            std::vector<wxTerminal::InputAtom>& out) {
    PromptGfx st;
    st.fg = defFg;
    st.bg = defBg;
    size_t i = 0;
    while (i < p.size()) {
        const auto b = static_cast<unsigned char>(p[i]);
        if (b == 0x1bu) {
            i = ConsumeEsc(p, i, st, defFg, defBg);
            continue;
        }
        if (b == '\n') {
            wxTerminal::InputAtom na{};
            na.is_newline = true;
            na.fg = st.fg;
            na.bg = st.bg;
            na.bold = st.bold;
            na.italic = st.italic;
            na.inverse = st.inverse;
            out.push_back(na);
            ++i;
            continue;
        }
        std::string_view rest = p.substr(i);
        uint32_t cp = 0;
        bool ok = false;
        Utf8Consume(rest, cp, ok);
        if (!ok) {
            ++i;
            continue;
        }
        i = p.size() - rest.size();
        wxTerminal::InputAtom a{};
        a.ch = wxUniChar(cp);
        {
            const int cw = CodepointColumnWidth(cp);
            a.col_width = cw < 1 ? 1 : (cw > 2 ? 2 : cw);
        }
        a.fg = st.fg;
        a.bg = st.bg;
        a.bold = st.bold;
        a.italic = st.italic;
        a.inverse = st.inverse;
        out.push_back(a);
    }
}

} // namespace

wxTerminal::wxTerminal(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                       long style)
    : wxPanel(parent, id, pos, size, style | wxWANTS_CHARS | wxBORDER_SUNKEN)
    , m_caretTimer(this, ID_CARET_TIMER) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    SetName(wxT("wxTerminal"));
    SetCanFocus(true);

    m_defaultFontPt = 11;
    m_font = wxFont(m_defaultFontPt, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    SetFont(m_font);

    // m_vscroll = new NoFocusScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
    // // Scrollbar must not steal keyboard focus (breaks caret + typing).
    // m_vscroll->SetCanFocus(false);
    // m_vscroll->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& e) {
    //     SetFocus();
    //     e.Skip(false);
    // });
    // m_vscroll->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
    //     // Keep focus on terminal even while user drags scrollbar.
    //     SetFocus();
    //     e.Skip();
    // });
    // // If key events still land on the scrollbar, reroute them to the terminal.
    // auto rerouteKey = [this](wxKeyEvent& e) {
    //     SetFocus();
    //     wxKeyEvent fwd(e);
    //     fwd.SetEventObject(this);
    //     GetEventHandler()->ProcessEvent(fwd);
    //     e.Skip(false);
    // };
    // m_vscroll->Bind(wxEVT_CHAR_HOOK, rerouteKey);
    // m_vscroll->Bind(wxEVT_KEY_DOWN, rerouteKey);
    // m_vscroll->Bind(wxEVT_CHAR, rerouteKey);
    // m_vscroll->SetScrollbar(0, 1, 1, 1, false);
    // const int sid = m_vscroll->GetId();
    // Bind(wxEVT_SCROLL_TOP, &wxTerminal::OnVScroll, this, sid);
    // Bind(wxEVT_SCROLL_BOTTOM, &wxTerminal::OnVScroll, this, sid);
    // Bind(wxEVT_SCROLL_LINEUP, &wxTerminal::OnVScroll, this, sid);
    // Bind(wxEVT_SCROLL_LINEDOWN, &wxTerminal::OnVScroll, this, sid);
    // Bind(wxEVT_SCROLL_PAGEUP, &wxTerminal::OnVScroll, this, sid);
    // Bind(wxEVT_SCROLL_PAGEDOWN, &wxTerminal::OnVScroll, this, sid);
    // Bind(wxEVT_SCROLL_THUMBTRACK, &wxTerminal::OnVScroll, this, sid);
    // Bind(wxEVT_SCROLL_THUMBRELEASE, &wxTerminal::OnVScroll, this, sid);

    // GTK may focus internal child windows of wxScrollBar (not the wxScrollBar itself).
    // Catch all child-focus transitions and keep focus on the terminal.
    // Bind(wxEVT_CHILD_FOCUS, [this](wxChildFocusEvent& e) {
    //     wxWindow* w = e.GetWindow();
    //     bool fromScrollbar = false;
    //     for (wxWindow* p = w; p; p = p->GetParent()) {
    //         if (p == m_vscroll) {
    //             fromScrollbar = true;
    //             break;
    //         }
    //     }
    //     if (fromScrollbar) {
    //         SetFocus();
    //         e.Skip(false);
    //         return;
    //     }
    //     e.Skip();
    // });

    m_bufRows = m_visibleRows + kScrollbackExtraRows;
    m_grid.assign(static_cast<size_t>(m_bufRows * m_cols), Cell{});

    m_interpreters.push_back(std::make_unique<term::XtermInterpreter>(*this));

    Bind(wxEVT_PAINT, &wxTerminal::OnPaint, this);
    Bind(wxEVT_SIZE, &wxTerminal::OnSize, this);
    Bind(wxEVT_CHAR, &wxTerminal::OnChar, this);
    Bind(wxEVT_KEY_DOWN, &wxTerminal::OnKeyDown, this);
    Bind(wxEVT_SET_FOCUS, &wxTerminal::OnSetFocus, this);
    Bind(wxEVT_KILL_FOCUS, &wxTerminal::OnKillFocus, this);
    Bind(wxEVT_TIMER, &wxTerminal::OnCaretTimer, this, ID_CARET_TIMER);
    Bind(wxEVT_LEFT_DOWN, &wxTerminal::OnMouseDown, this);
    // If user selects text with right/middle click, ensure the terminal still gets
    // keyboard focus so typing keeps working.
    Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent& e) {
        SetFocus();
        e.Skip();
    });
    Bind(wxEVT_MIDDLE_DOWN, [this](wxMouseEvent& e) {
        SetFocus();
        e.Skip();
    });
    Bind(wxEVT_MOTION, &wxTerminal::OnMouseMove, this);
    Bind(wxEVT_LEFT_UP, &wxTerminal::OnMouseUp, this);
    Bind(wxEVT_MOUSEWHEEL, &wxTerminal::OnMouseWheel, this);
    m_caretTimer.Start(530);
    RecalcGrid();
    SyncScrollbarFromView();
}

wxTerminal::~wxTerminal() {
    if (m_topKeyHookWnd) {
        // We bound a lambda; Unbind() can't remove it reliably without storing it.
        // Leaving it would be dangerous if it captured `this`, so we guard by disabling the hook
        // via clearing the window pointer and returning early in the lambda's focus walk.
        // (The window will typically outlive this terminal only briefly during teardown.)
        m_topKeyHookWnd = nullptr;
    }
}

void wxTerminal::SetForegroundPtyPassthrough(bool on) {
    m_foregroundPtyPassthrough = on;
    Refresh();
}

void wxTerminal::AddInterpreter(std::unique_ptr<TermInterpreter> interp) {
    if (interp)
        m_interpreters.insert(m_interpreters.begin(), std::move(interp));
}

void wxTerminal::ResetInterpreters() {
    for (auto& p : m_interpreters) {
        if (p)
            p->Reset();
    }
}

void wxTerminal::EscSecond(uint8_t ch) {
    if (ch == static_cast<uint8_t>('c'))
        ClearScreen();
}

void wxTerminal::SetPrompt(const wxString& prompt) {
    wxScopedCharBuffer b = prompt.utf8_str();
    const char* p = b.data();
    if (p)
        SetPromptUtf8(std::string_view(p, b.length()));
    else
        SetPromptUtf8("");
}

void wxTerminal::SetPromptUtf8(std::string_view utf8) {
    m_promptUtf8.assign(utf8.data(), utf8.size());
    m_inputAtomsDirty = true;
    Refresh();
}

wxString wxTerminal::GetPrompt() const {
    return wxString::FromUTF8(m_promptUtf8.data(), m_promptUtf8.size());
}

void wxTerminal::EnsureInputAtoms() const {
    if (!m_inputAtomsDirty)
        return;
    wxTerminal* t = const_cast<wxTerminal*>(this);
    t->RebuildInputLineAtoms();
}

void wxTerminal::RebuildInputLineAtoms() {
    m_inputAtoms.clear();
    ParsePromptUtf8(std::string_view(m_promptUtf8), m_defFg, m_defBg, m_inputAtoms);
    m_promptAtomCount = m_inputAtoms.size();
    for (size_t i = 0; i < m_edit.length(); ++i) {
        InputAtom a{};
        a.ch = m_edit[i];
        {
            const uint32_t cp = static_cast<uint32_t>(m_edit[i].GetValue());
            const int cw = CodepointColumnWidth(cp);
            a.col_width = cw < 1 ? 1 : (cw > 2 ? 2 : cw);
        }
        a.fg = m_defFg;
        a.bg = m_defBg;
        m_inputAtoms.push_back(a);
    }
    m_inputAtomsDirty = false;
}

void wxTerminal::MapFlatPosToCell(size_t pos, int* br, int* bc) const {
    if (!br || !bc)
        return;
    EnsureInputAtoms();
    int bbr = m_cv, bbc = m_cu;
    for (size_t k = 0; k < pos && k < m_inputAtoms.size(); ++k) {
        const InputAtom& at = m_inputAtoms[k];
        if (at.is_newline) {
            bbr++;
            bbc = 0;
            continue;
        }
        const int w = std::max(1, at.col_width);
        bbc += w;
        if (bbc >= m_cols) {
            bbc = 0;
            bbr++;
        }
    }
    *br = bbr;
    *bc = bbc;
}

wxString wxTerminal::PromptPlainForEcho() const {
    EnsureInputAtoms();
    wxString acc;
    for (size_t i = 0; i < m_promptAtomCount && i < m_inputAtoms.size(); ++i) {
        const InputAtom& a = m_inputAtoms[i];
        if (a.is_newline)
            acc += wxChar('\n');
        else
            acc += a.ch;
    }
    return acc;
}

void wxTerminal::ClearScreen() {
    m_grid.assign(static_cast<size_t>(m_bufRows * m_cols), Cell{});
    m_cu = 0;
    m_cv = 0;
    m_viewTop = 0;
    m_followOutput = true;
    m_curFg = m_defFg;
    m_curBg = m_defBg;
    m_curBold = false;
    m_curItalic = false;
    m_curInverse = false;
    ResetInterpreters();
    SyncScrollbarFromView();
    Refresh();
}

void wxTerminal::SetFgColor(const wxColour& c) {
    m_curFg = c;
}

void wxTerminal::SetBgColor(const wxColour& c) {
    m_curBg = c;
}

void wxTerminal::SetBold(bool on) {
    m_curBold = on;
}

void wxTerminal::SetItalic(bool on) {
    m_curItalic = on;
}

void wxTerminal::SetInverse(bool on) {
    m_curInverse = on;
}

void wxTerminal::ResetAttributes() {
    m_curFg = m_defFg;
    m_curBg = m_defBg;
    m_curBold = false;
    m_curItalic = false;
    m_curInverse = false;
}

void wxTerminal::SetCursorVisible(bool visible) {
    m_cursorVisible = visible;
    Refresh();
}

void wxTerminal::GoUp(int delta) {
    if (delta < 1)
        delta = 1;
    m_cv = std::max(0, m_cv - delta);
}

void wxTerminal::GoDown(int delta) {
    if (delta < 1)
        delta = 1;
    m_cv = std::min(m_bufRows - 1, m_cv + delta);
}

void wxTerminal::GoRight(int delta) {
    if (delta < 1)
        delta = 1;
    m_cu = std::min(m_cols - 1, m_cu + delta);
}

void wxTerminal::GoLeft(int delta) {
    if (delta < 1)
        delta = 1;
    m_cu = std::max(0, m_cu - delta);
}

void wxTerminal::SetCursorRow(int row1Based) {
    int row = row1Based - 1;
    if (row < 0)
        row = 0;
    if (row >= m_bufRows)
        row = m_bufRows - 1;
    m_cv = row;
}

void wxTerminal::SetCursorColumn(int col1Based) {
    int col = col1Based - 1;
    if (col < 0)
        col = 0;
    if (col >= m_cols)
        col = m_cols - 1;
    m_cu = col;
}

void wxTerminal::SetCursorPosition(int row1Based, int col1Based) {
    SetCursorRow(row1Based);
    SetCursorColumn(col1Based);
}

void wxTerminal::EraseInDisplay(int mode) {
    const int total = m_bufRows * m_cols;
    int idx = m_cv * m_cols + m_cu;
    if (mode == 0) {
        for (int i = idx; i < total; ++i)
            m_grid[static_cast<size_t>(i)] = Cell{};
    } else if (mode == 1) {
        for (int i = 0; i <= idx; ++i)
            m_grid[static_cast<size_t>(i)] = Cell{};
    } else {
        m_grid.assign(static_cast<size_t>(total), Cell{});
    }
}

void wxTerminal::EraseInLine(int mode) {
    int rowStart = m_cv * m_cols;
    if (mode == 0) {
        for (int c = m_cu; c < m_cols; ++c)
            m_grid[static_cast<size_t>(rowStart + c)] = Cell{};
    } else if (mode == 1) {
        for (int c = 0; c <= m_cu; ++c)
            m_grid[static_cast<size_t>(rowStart + c)] = Cell{};
    } else {
        for (int c = 0; c < m_cols; ++c)
            m_grid[static_cast<size_t>(rowStart + c)] = Cell{};
    }
}

wxColour wxTerminal::AnsiFg(int code) {
    static const wxColour tbl[8] = {wxColour(0, 0, 0),       wxColour(205, 49, 49),
                                    wxColour(13, 179, 0),    wxColour(229, 193, 0),
                                    wxColour(36, 114, 200),  wxColour(188, 63, 188),
                                    wxColour(17, 168, 171),  wxColour(229, 229, 229)};
    if (code >= 30 && code <= 37)
        return tbl[code - 30];
    return m_defFg;
}

wxColour wxTerminal::AnsiBg(int code) {
    static const wxColour tbl[8] = {wxColour(0, 0, 0),       wxColour(205, 49, 49),
                                    wxColour(13, 179, 0),    wxColour(229, 193, 0),
                                    wxColour(36, 114, 200),  wxColour(188, 63, 188),
                                    wxColour(17, 168, 171),  wxColour(229, 229, 229)};
    if (code >= 40 && code <= 47)
        return tbl[code - 40];
    return m_defBg;
}

void wxTerminal::ApplySgr(const std::vector<int>& args) {
    if (args.empty()) {
        ResetAttributes();
        return;
    }
    for (size_t i = 0; i < args.size(); ++i) {
        int v = args[i];
        if (v == 0) {
            ResetAttributes();
        } else if (v == 1) {
            m_curBold = true;
        } else if (v == 3) {
            m_curItalic = true;
        } else if (v == 7) {
            m_curInverse = true;
        } else if (v == 22) {
            m_curBold = false;
        } else if (v == 23) {
            m_curItalic = false;
        } else if (v == 27) {
            m_curInverse = false;
        } else if (v >= 30 && v <= 37) {
            m_curFg = AnsiFg(v);
        } else if (v == 39) {
            m_curFg = m_defFg;
        } else if (v >= 40 && v <= 47) {
            m_curBg = AnsiBg(v);
        } else if (v == 49) {
            m_curBg = m_defBg;
        }
    }
}

void wxTerminal::RecalcGrid() {
    wxClientDC dc(this);
    dc.SetFont(m_font);
    int cw = 0, ch = 0;
    dc.GetTextExtent(wxS("M"), &cw, &ch, nullptr, nullptr, &m_font);
    if (cw < 4)
        cw = 8;
    if (ch < 8)
        ch = 16;
    m_cellW = cw;
    m_cellH = ch;

    wxSize sz = GetClientSize();
    const int vpW = ViewportWidthPx();
    /* Before first layout, width is often 0 — do not force m_cols to kMinCols (ugly wrapping). */
    if (vpW <= 0) {
        Refresh();
        return;
    }

    int usableH = std::max(m_cellH * kMinRows, sz.GetHeight());
    int newVisible = std::max(kMinRows, usableH / m_cellH);
    int newCols = std::max(kMinCols, vpW / m_cellW);
    int newBufRows = newVisible + kScrollbackExtraRows;

    if (newCols == m_cols && newVisible == m_visibleRows && newBufRows == m_bufRows)
        return;

    std::vector<Cell> old = std::move(m_grid);
    const int oc = m_cols;
    const int obr = m_bufRows;

    m_cols = newCols;
    m_visibleRows = newVisible;
    m_bufRows = newBufRows;
    m_grid.assign(static_cast<size_t>(m_bufRows * m_cols), Cell{});

    const int copyR = std::min(obr, m_bufRows);
    const int copyC = std::min(oc, m_cols);
    for (int r = 0; r < copyR; ++r) {
        for (int c = 0; c < copyC; ++c)
            BufCell(r, c) = old[static_cast<size_t>(r * oc + c)];
    }
    if (m_cu >= m_cols)
        m_cu = m_cols - 1;
    if (m_cv >= m_bufRows)
        m_cv = m_bufRows - 1;
    const int maxTop = std::max(0, m_bufRows - m_visibleRows);
    if (m_viewTop > maxTop)
        m_viewTop = maxTop;
    m_inputAtomsDirty = true;
    Refresh();
    SyncScrollbarFromView();
}

void wxTerminal::OnSize(wxSizeEvent& e) {
    if (m_vscroll) {
        const wxSize sz = GetClientSize();
        m_scrollbarW = std::clamp(m_scrollbarW, 1, sz.GetWidth());
        m_vscroll->SetPosition(wxPoint(sz.GetWidth() - m_scrollbarW, 0));
        m_vscroll->SetSize(wxSize(m_scrollbarW, sz.GetHeight()));
        m_vscroll->Show(true);
    }
    RecalcGrid();
    e.Skip();
}

void wxTerminal::ScrollBufferUp() {
    for (int r = 1; r < m_bufRows; ++r) {
        for (int c = 0; c < m_cols; ++c)
            BufCell(r - 1, c) = BufCell(r, c);
    }
    for (int c = 0; c < m_cols; ++c)
        BufCell(m_bufRows - 1, c) = Cell{};
    if (m_cv > 0)
        --m_cv;
    if (m_viewTop > 0)
        --m_viewTop;
    SyncScrollbarFromView();
}

void wxTerminal::CarriageReturn() {
    m_cu = 0;
}

void wxTerminal::LineFeed() {
    if (m_cv + 1 >= m_bufRows)
        ScrollBufferUp();
    else
        ++m_cv;
}

void wxTerminal::Tab() {
    m_cu = ((m_cu / 8) + 1) * 8;
    if (m_cu >= m_cols) {
        m_cu = 0;
        LineFeed();
    }
}

void wxTerminal::Receive(uint32_t cp) {
    for (auto& interp : m_interpreters) {
        if (interp && interp->Receive(cp))
            return;
    }
    PutChar(cp);
}

void wxTerminal::PutChar(uint32_t cp) {
    if (cp == '\r') {
        CarriageReturn();
        return;
    }
    if (cp == '\n') {
        /* Unix text often uses LF only; treat as newline + return to column 0 (avoid stair-step). */
        CarriageReturn();
        LineFeed();
        return;
    }
    if (cp == '\b' || cp == 0x7f) {
        if (m_cu > 0) {
            const int col = m_cu - 1;
            const WidePortion w = BufCell(m_cv, col).wide;
            if (w == WidePortion::Tail && col > 0) {
                const int h = col - 1;
                BufCell(m_cv, h) = Cell{};
                BufCell(m_cv, col) = Cell{};
                m_cu = h;
            } else if (w == WidePortion::Head) {
                BufCell(m_cv, col) = Cell{};
                if (col + 1 < m_cols)
                    BufCell(m_cv, col + 1) = Cell{};
                m_cu = col;
            } else {
                BufCell(m_cv, col) = Cell{};
                m_cu = col;
            }
        }
        return;
    }
    if (cp == '\t') {
        Tab();
        return;
    }
    if (cp < 32u && cp != '\t')
        return;

    int cw = CodepointColumnWidth(cp);
    if (cw < 1)
        cw = 1;
    if (cw > 2)
        cw = 2;

    wxUniChar uch(static_cast<wxUint32>(cp));
    Cell cell{};
    cell.ch = uch;
    cell.fg = m_curFg;
    cell.bg = m_curBg;
    cell.bold = m_curBold;
    cell.italic = m_curItalic;
    cell.inverse = m_curInverse;

    if (cw == 2) {
        if (m_cu + 1 >= m_cols) {
            m_cu = 0;
            LineFeed();
        }
        if (m_cu + 1 >= m_cols) {
            m_cu = 0;
            LineFeed();
        }
        cell.wide = WidePortion::Head;
        BufCell(m_cv, m_cu) = cell;
        Cell tail{};
        tail.ch = wxUniChar(' ');
        tail.wide = WidePortion::Tail;
        tail.fg = m_curFg;
        tail.bg = m_curBg;
        tail.bold = m_curBold;
        tail.italic = m_curItalic;
        tail.inverse = m_curInverse;
        BufCell(m_cv, m_cu + 1) = tail;
        m_cu += 2;
    } else {
        if (m_cu >= m_cols) {
            m_cu = 0;
            LineFeed();
        }
        BufCell(m_cv, m_cu) = cell;
        ++m_cu;
    }
    if (m_cu >= m_cols) {
        m_cu = 0;
        LineFeed();
    }
}

void wxTerminal::PutChars(std::string_view utf8) {
    ForEachUtf8Cp(utf8, [this](uint32_t cp) { PutChar(cp); });
}

void wxTerminal::WriteUtf8(std::string_view utf8) {
    ForEachUtf8Cp(utf8, [this](uint32_t cp) { Receive(cp); });
    if (m_followOutput)
        ScrollToFollowOutputCursor();
    Refresh();
}

void wxTerminal::WriteUtf8BelowInputOverlay(std::string_view utf8) {
    EnsureInputAtoms();
    const int savR = m_cv;
    const int savC = m_cu;
    const int rowBelow = LastInputBufRow() + 1;
    m_cv = rowBelow;
    m_cu = 0;
    ForEachUtf8Cp(utf8, [this](uint32_t cp) { Receive(cp); });
    const int postCv = m_cv;
    m_cv = savR;
    m_cu = savC;
    if (m_followOutput) {
        const int bottom = std::max(postCv, LastInputBufRow());
        int newTop = bottom - m_visibleRows + 1;
        if (newTop < 0)
            newTop = 0;
        const int maxTop = std::max(0, m_bufRows - m_visibleRows);
        if (newTop > maxTop)
            newTop = maxTop;
        m_viewTop = newTop;
    }
    SyncScrollbarFromView();
    Refresh();
}

int wxTerminal::LastInputBufRow() const {
    EnsureInputAtoms();
    int br = m_cv, bc = m_cu;
    for (const InputAtom& a : m_inputAtoms) {
        if (a.is_newline) {
            br++;
            bc = 0;
            continue;
        }
        bc += std::max(1, a.col_width);
        if (bc >= m_cols) {
            bc = 0;
            br++;
        }
    }
    return br;
}

void wxTerminal::ScrollToFollowOutputCursor() {
    const int bottom = std::max(m_cv, LastInputBufRow());
    int newTop = bottom - m_visibleRows + 1;
    if (newTop < 0)
        newTop = 0;
    const int maxTop = std::max(0, m_bufRows - m_visibleRows);
    if (newTop > maxTop)
        newTop = maxTop;
    m_viewTop = newTop;
    SyncScrollbarFromView();
}

void wxTerminal::ScrollViewByPages(int deltaPages) {
    const int page = std::max(1, m_visibleRows - 1);
    const int maxTop = std::max(0, m_bufRows - m_visibleRows);
    m_viewTop += deltaPages * page;
    if (m_viewTop < 0)
        m_viewTop = 0;
    if (m_viewTop > maxTop)
        m_viewTop = maxTop;
    const int bottom = std::max(m_cv, LastInputBufRow());
    m_followOutput = (m_viewTop + m_visibleRows - 1 >= bottom);
    SyncScrollbarFromView();
    Refresh();
}

void wxTerminal::ScrollViewByLines(int deltaLines) {
    if (deltaLines == 0)
        return;
    const int maxTop = std::max(0, m_bufRows - m_visibleRows);
    m_viewTop += deltaLines;
    if (m_viewTop < 0)
        m_viewTop = 0;
    if (m_viewTop > maxTop)
        m_viewTop = maxTop;
    const int bottom = std::max(m_cv, LastInputBufRow());
    m_followOutput = (m_viewTop + m_visibleRows - 1 >= bottom);
    SyncScrollbarFromView();
    Refresh();
}

bool wxTerminal::HitTestVisibleCell(const wxPoint& clientPt, int* visRow, int* col) const {
    if (!visRow || !col)
        return false;
    if (clientPt.y < 0 || clientPt.y >= m_visibleRows * m_cellH)
        return false;
    if (clientPt.x < 0 || clientPt.x >= ViewportWidthPx())
        return false;
    *visRow = clientPt.y / m_cellH;
    *col = clientPt.x / m_cellW;
    if (*visRow < 0)
        *visRow = 0;
    if (*visRow >= m_visibleRows)
        *visRow = m_visibleRows - 1;
    if (*col < 0)
        *col = 0;
    if (*col >= m_cols)
        *col = m_cols - 1;
    return true;
}

bool wxTerminal::HitTestInputLine(const wxPoint& clientPt, int* charIndex) const {
    if (!charIndex)
        return false;
    if (clientPt.x < 0 || clientPt.x >= ViewportWidthPx())
        return false;
    EnsureInputAtoms();
    int br = m_cv, bc = m_cu;
    for (size_t i = 0; i <= m_inputAtoms.size(); ++i) {
        const int vr = br - m_viewTop;
        if (vr >= 0 && vr < m_visibleRows && bc >= 0 && bc < m_cols) {
            const int x0 = bc * m_cellW;
            const int y0 = vr * m_cellH;
            int wcells = 1;
            if (i < m_inputAtoms.size() && !m_inputAtoms[i].is_newline)
                wcells = std::max(1, m_inputAtoms[i].col_width);
            const int x1 = x0 + m_cellW * wcells;
            const int y1 = y0 + m_cellH;
            if (clientPt.x >= x0 && clientPt.x < x1 && clientPt.y >= y0 && clientPt.y < y1) {
                *charIndex = static_cast<int>(i);
                return true;
            }
        }
        if (i < m_inputAtoms.size()) {
            const InputAtom& at = m_inputAtoms[i];
            if (at.is_newline) {
                br++;
                bc = 0;
            } else {
                bc += std::max(1, at.col_width);
                if (bc >= m_cols) {
                    bc = 0;
                    br++;
                }
            }
        }
    }
    return false;
}

void wxTerminal::ClearSelection() {
    m_hasSelection = false;
    m_dragged = false;
}

void wxTerminal::NormalizeGridSel(int& r0, int& c0, int& r1, int& c1) const {
    if (r0 > r1 || (r0 == r1 && c0 > c1)) {
        std::swap(r0, r1);
        std::swap(c0, c1);
    }
}

void wxTerminal::NormalizePromptSel(int& a, int& b) const {
    if (a > b)
        std::swap(a, b);
}

wxString wxTerminal::TextFromGridSelection() const {
    int r0 = m_selGrR0, c0 = m_selGrC0, r1 = m_selGrR1, c1 = m_selGrC1;
    NormalizeGridSel(r0, c0, r1, c1);
    wxString acc;
    for (int r = r0; r <= r1; ++r) {
        int cs = (r == r0) ? c0 : 0;
        int ce = (r == r1) ? c1 : (m_cols - 1);
        wxString line;
        for (int c = cs; c <= ce; ++c) {
            const Cell& cell = BufCell(r, c);
            if (cell.wide == WidePortion::Tail)
                continue;
            line += wxString(cell.ch);
        }
        RTrimLine(line);
        acc += line;
        if (r < r1)
            acc += wxS("\n");
    }
    return acc;
}

wxString wxTerminal::TextFromPromptSelection() const {
    int a = m_selPrStart, b = m_selPrEnd;
    NormalizePromptSel(a, b);
    EnsureInputAtoms();
    if (m_inputAtoms.empty() || a >= static_cast<int>(m_inputAtoms.size()))
        return wxString();
    b = std::min(b, static_cast<int>(m_inputAtoms.size()) - 1);
    wxString acc;
    for (int k = a; k <= b; ++k) {
        const InputAtom& at = m_inputAtoms[static_cast<size_t>(k)];
        if (at.is_newline)
            acc += wxChar('\n');
        else
            acc += at.ch;
    }
    return acc;
}

void wxTerminal::CopySelection() {
    if (!m_hasSelection)
        return;
    wxString text = m_selIsGrid ? TextFromGridSelection() : TextFromPromptSelection();
    if (text.empty())
        return;
    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
    }
}

void wxTerminal::CutSelection() {
    if (!m_hasSelection)
        return;
    CopySelection();
    if (m_selIsGrid)
        return;

    int a = m_selPrStart, b = m_selPrEnd;
    NormalizePromptSel(a, b);
    EnsureInputAtoms();
    const size_t plen = m_promptAtomCount;
    const int editLen = static_cast<int>(m_inputAtoms.size() - plen);
    const int editStart = static_cast<int>(plen);
    const int editEnd = editStart + std::max(0, editLen - 1);
    const int cut0 = std::max(a, editStart);
    const int cut1 = std::min(b, editEnd);
    if (cut0 > cut1)
        return;

    const size_t off0 = static_cast<size_t>(cut0 - editStart);
    const size_t n = static_cast<size_t>(cut1 - cut0 + 1);
    m_edit.erase(off0, n);

    if (m_editCaret >= off0 + n)
        m_editCaret -= n;
    else if (m_editCaret > off0)
        m_editCaret = off0;

    ClearSelection();
    m_inputAtomsDirty = true;
    Refresh();
}

void wxTerminal::PasteToForegroundPty() {
    if (!OnSendForegroundPty)
        return;
    if (!wxTheClipboard->Open())
        return;
    wxTextDataObject data;
    if (!wxTheClipboard->GetData(data)) {
        wxTheClipboard->Close();
        return;
    }
    wxTheClipboard->Close();
    wxString t = data.GetText();
    if (t.empty())
        return;
    wxScopedCharBuffer b = t.utf8_str();
    const char* p = b.data();
    if (!p)
        return;
    std::string s(p, b.length());
    for (char& c : s) {
        if (c == '\n')
            c = '\r';
    }
    OnSendForegroundPty(s);
}

void wxTerminal::PasteFromClipboard() {
    if (m_foregroundPtyPassthrough && OnSendForegroundPty) {
        PasteToForegroundPty();
        return;
    }
    if (!wxTheClipboard->Open())
        return;
    wxTextDataObject data;
    if (!wxTheClipboard->GetData(data)) {
        wxTheClipboard->Close();
        return;
    }
    wxTheClipboard->Close();
    wxString t = data.GetText();
    if (t.empty())
        return;
    for (size_t i = 0; i < t.length(); ++i) {
        const wxChar u = t[i];
        if (u == '\r' || u == '\n' || u == '\t')
            continue;
        if (u < 32 || u == 127)
            continue;
        if (m_editCaret > m_edit.length())
            m_editCaret = m_edit.length();
        m_edit.insert(m_editCaret, 1, u);
        ++m_editCaret;
    }
    m_inputAtomsDirty = true;
    m_followOutput = true;
    ScrollToFollowOutputCursor();
    Refresh();
}

bool wxTerminal::IsGridCellSelected(int visRow, int col) const {
    if (!m_hasSelection || !m_selIsGrid)
        return false;
    const int bufRow = m_viewTop + visRow;
    int r0 = m_selGrR0, c0 = m_selGrC0, r1 = m_selGrR1, c1 = m_selGrC1;
    NormalizeGridSel(r0, c0, r1, c1);
    if (bufRow < r0 || bufRow > r1 || col < 0 || col >= m_cols)
        return false;
    const int cs = (bufRow == r0) ? c0 : 0;
    const int ce = (bufRow == r1) ? c1 : (m_cols - 1);
    int g = col;
    if (BufCell(bufRow, col).wide == WidePortion::Tail)
        g = col - 1;
    if (g < 0)
        g = 0;
    int ge = g;
    if (BufCell(bufRow, g).wide == WidePortion::Head)
        ge = g + 1;
    return ge >= cs && g <= ce;
}

void wxTerminal::OnMouseDown(wxMouseEvent& e) {
    SetFocus();
    m_dragged = false;
    int vrow = 0, col = 0, pr = 0;
    if (HitTestInputLine(e.GetPosition(), &pr)) {
        m_selIsGrid = false;
        m_mouseDown = true;
        m_selPrStart = m_selPrEnd = pr;
        m_hasSelection = false;
        CaptureMouse();
    } else if (HitTestVisibleCell(e.GetPosition(), &vrow, &col)) {
        m_selIsGrid = true;
        m_mouseDown = true;
        const int br = m_viewTop + vrow;
        m_selGrR0 = m_selGrR1 = br;
        m_selGrC0 = m_selGrC1 = col;
        m_hasSelection = false;
        CaptureMouse();
    }
    Refresh();
}

void wxTerminal::OnMouseMove(wxMouseEvent& e) {
    if (!m_mouseDown)
        return;
    m_dragged = true;
    int vrow = 0, col = 0, pr = 0;
    if (m_selIsGrid && HitTestVisibleCell(e.GetPosition(), &vrow, &col)) {
        m_selGrR1 = m_viewTop + vrow;
        m_selGrC1 = col;
        m_hasSelection = true;
        Refresh();
    } else if (!m_selIsGrid && HitTestInputLine(e.GetPosition(), &pr)) {
        m_selPrEnd = pr;
        m_hasSelection = true;
        Refresh();
    }
}

void wxTerminal::OnMouseUp(wxMouseEvent& e) {
    // After selections via mouse (including right/middle click),
    // ensure keyboard focus is back on the terminal.
    SetFocus();
    if (!m_mouseDown)
        return;
    m_mouseDown = false;
    if (HasCapture())
        ReleaseMouse();

    if (m_selIsGrid) {
        if (m_dragged)
            m_hasSelection = true;
        else {
            ClearSelection();
        }
    } else {
        if (m_dragged)
            m_hasSelection = true;
        else {
            int pr = 0;
            if (HitTestInputLine(e.GetPosition(), &pr)) {
                EnsureInputAtoms();
                const int pl = static_cast<int>(m_promptAtomCount);
                if (pr <= pl)
                    m_editCaret = 0;
                else {
                    const int editAtoms = static_cast<int>(m_inputAtoms.size() - pl);
                    m_editCaret = static_cast<size_t>(
                        std::max(0, std::min(pr - pl, editAtoms)));
                }
            }
            ClearSelection();
        }
    }
    Refresh();
}

void wxTerminal::OnMouseWheel(wxMouseEvent& e) {
    int delta = e.GetWheelDelta();
    if (delta == 0)
        delta = 120;
    int lines = e.GetLinesPerAction();
    if (lines < 1)
        lines = 3;
    const int steps = (e.GetWheelRotation() * lines) / delta;
    if (steps != 0)
        ScrollViewByLines(-steps);
}

void wxTerminal::DrawInputLine(wxDC& dc) {
    EnsureInputAtoms();
    int br = m_cv, bc = m_cu;
    int a = 0, b = 0;
    const bool sel = m_hasSelection && !m_selIsGrid;
    if (sel) {
        a = m_selPrStart;
        b = m_selPrEnd;
        NormalizePromptSel(a, b);
    }
    for (size_t i = 0; i < m_inputAtoms.size(); ++i) {
        const InputAtom& at = m_inputAtoms[i];
        const int vr = br - m_viewTop;
        const bool inSel = sel && static_cast<int>(i) >= a && static_cast<int>(i) <= b;
        if (at.is_newline) {
            br++;
            bc = 0;
            continue;
        }
        wxColour fg = at.fg;
        wxColour bg = at.bg;
        if (at.inverse)
            std::swap(fg, bg);
        if (inSel)
            bg = BlendTowards(bg, kSelTint, 120);
        const int wcells = std::max(1, at.col_width);
        if (vr >= 0 && vr < m_visibleRows && bc >= 0 && bc < m_cols) {
            const int x = bc * m_cellW;
            const int y = vr * m_cellH;
            dc.SetBrush(wxBrush(bg));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(x, y, m_cellW * wcells, m_cellH);
            dc.SetTextForeground(fg);
            wxFont drawFont(m_font);
            if (at.bold)
                drawFont.SetWeight(wxFONTWEIGHT_BOLD);
            if (at.italic)
                drawFont.SetStyle(wxFONTSTYLE_ITALIC);
            dc.SetFont(drawFont);
            dc.DrawText(wxString(at.ch), x + 1, y);
        }
        bc += wcells;
        if (bc >= m_cols) {
            bc = 0;
            br++;
        }
    }
}

void wxTerminal::DrawInputCaret(wxDC& dc) {
    if (!m_cursorVisible || !m_caretOn || FindFocus() != this)
        return;
    EnsureInputAtoms();
    const size_t caretPos = m_promptAtomCount + m_editCaret;
    int br = 0, bc = 0;
    MapFlatPosToCell(caretPos, &br, &bc);
    const int vr = br - m_viewTop;
    if (vr < 0 || vr >= m_visibleRows || bc < 0 || bc >= m_cols)
        return;
    const int cx = bc * m_cellW + 1;
    const int cy = vr * m_cellH;
    const bool block = !m_insertMode;
    if (block) {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour(0, 0, 0), 1));
        wxString ch;
        int cwAtoms = 1;
        if (caretPos < m_inputAtoms.size()) {
            const InputAtom& ca = m_inputAtoms[caretPos];
            if (!ca.is_newline)
                cwAtoms = std::max(1, ca.col_width);
        }
        if (m_editCaret < m_edit.length())
            ch = m_edit[static_cast<size_t>(m_editCaret)];
        else
            ch = wxS(" ");
        wxSize chExt = dc.GetTextExtent(ch);
        const int cww = std::max(std::max(2, chExt.GetWidth()), m_cellW * cwAtoms);
        dc.DrawRectangle(cx, cy + 1, cww, m_cellH - 2);
    } else {
        dc.SetPen(wxPen(wxColour(0, 120, 215), 2));
        dc.DrawLine(cx, cy + 2, cx, cy + m_cellH - 2);
    }
}

void wxTerminal::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetFont(m_font);
    wxSize sz = GetClientSize();
    dc.SetBrush(wxBrush(wxColour(245, 245, 245)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, sz.GetWidth(), sz.GetHeight());

    for (int vr = 0; vr < m_visibleRows; ++vr) {
        const int br = m_viewTop + vr;
        for (int c = 0; c < m_cols;) {
            const Cell& cell = BufCell(br, c);
            if (cell.wide == WidePortion::Tail) {
                ++c;
                continue;
            }
            int span = 1;
            if (cell.wide == WidePortion::Head && c + 1 < m_cols)
                span = 2;
            wxColour bg = cell.inverse ? cell.fg : cell.bg;
            wxColour fg = cell.inverse ? cell.bg : cell.fg;
            bool anySel = false;
            for (int t = 0; t < span; ++t) {
                if (IsGridCellSelected(vr, c + t))
                    anySel = true;
            }
            if (anySel)
                bg = BlendTowards(bg, kSelTint, 110);
            const int x = c * m_cellW;
            const int y = vr * m_cellH;
            dc.SetBrush(wxBrush(bg));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(x, y, m_cellW * span, m_cellH);
            dc.SetTextForeground(fg);
            wxString s(cell.ch);
            wxFont drawFont(m_font);
            if (cell.bold)
                drawFont.SetWeight(wxFONTWEIGHT_BOLD);
            if (cell.italic)
                drawFont.SetStyle(wxFONTSTYLE_ITALIC);
            dc.SetFont(drawFont);
            dc.DrawText(s, x + 1, y);
            c += span;
        }
    }

    if (!m_foregroundPtyPassthrough) {
        DrawInputLine(dc);
        DrawInputCaret(dc);
    }
}

void wxTerminal::OnChar(wxKeyEvent& e) {
    long k = e.GetKeyCode();
    if (m_foregroundPtyPassthrough && OnSendForegroundPty) {
        if (e.GetUnicodeKey() == 4 || k == 4) {
            OnSendForegroundPty("\x04");
            return;
        }
        if (k == WXK_TAB || e.GetUnicodeKey() == '\t')
            return;
        if (k == WXK_RETURN || k == WXK_NUMPAD_ENTER) {
            OnSendForegroundPty("\r");
            return;
        }
        const wxChar u = e.GetUnicodeKey();
        if (u && u >= 32 && u != 127) {
            wxString ws(u);
            wxScopedCharBuffer b = ws.utf8_str();
            const char* p = b.data();
            if (p && b.length() > 0)
                OnSendForegroundPty(std::string_view(p, b.length()));
            return;
        }
        e.Skip();
        return;
    }
    /* Ctrl+D → EOT (0x04): quit when input line is empty. */
    if (e.GetUnicodeKey() == 4 || k == 4) {
        if (m_edit.empty()) {
            if (OnEofEmptyLine)
                OnEofEmptyLine();
        }
        return;
    }
    if (k == WXK_TAB || e.GetUnicodeKey() == '\t')
        return;
    if (k == WXK_RETURN || k == WXK_NUMPAD_ENTER) {
        wxString line = m_edit;
        if (!line.empty()) {
            if (m_cmdHistory.empty() || m_cmdHistory.back() != line)
                m_cmdHistory.push_back(line);
        }
        m_historyNav = -1;
        m_historyDraft.clear();
        if (m_inputAtomsDirty)
            RebuildInputLineAtoms();
        wxString echo = wxS("\r") + PromptPlainForEcho() + m_edit + wxS("\r\n");
        m_edit.clear();
        m_editCaret = 0;
        m_inputAtomsDirty = true;
        ClearSelection();
        m_followOutput = true;
        {
            wxScopedCharBuffer buf = echo.utf8_str();
            WriteUtf8(std::string_view(buf.data(), buf.length()));
        }
        if (OnSubmitLine)
            OnSubmitLine(line);
        ScrollToFollowOutputCursor();
        Refresh();
        return;
    }
    wxChar u = e.GetUnicodeKey();
    if (u && u >= 32 && u != 127) {
        if (m_insertMode || m_editCaret >= m_edit.length()) {
            if (m_editCaret > m_edit.length())
                m_editCaret = m_edit.length();
            m_edit.insert(static_cast<size_t>(m_editCaret), 1, u);
            ++m_editCaret;
        } else {
            if (m_editCaret < m_edit.length()) {
                m_edit[static_cast<size_t>(m_editCaret)] = u;
                ++m_editCaret;
            } else {
                m_edit.append(1, u);
                ++m_editCaret;
            }
        }
        m_inputAtomsDirty = true;
        m_followOutput = true;
        ScrollToFollowOutputCursor();
        Refresh();
        return;
    }
    e.Skip();
}

void wxTerminal::OnKeyDown(wxKeyEvent& e) {
    const int k = e.GetKeyCode();
    const bool ctrl = e.ControlDown() || e.CmdDown();

    if (e.ShiftDown() && k == WXK_PAGEUP) {
        ScrollViewByPages(-1);
        return;
    }
    if (e.ShiftDown() && k == WXK_PAGEDOWN) {
        ScrollViewByPages(1);
        return;
    }

    if (m_foregroundPtyPassthrough && OnSendForegroundPty) {
        if (ctrl && e.ShiftDown() && (k == 'c' || k == 'C')) {
            CopySelection();
            return;
        }
        if (ctrl && (k == 'c' || k == 'C') && !e.ShiftDown()) {
            OnSendForegroundPty("\x03");
            return;
        }
        if (ctrl && (k == 'v' || k == 'V' || k == 22)) {
            PasteToForegroundPty();
            return;
        }
        if (ctrl && (k == 'd' || k == 'D' || k == 4)) {
            OnSendForegroundPty("\x04");
            return;
        }
        if (ctrl && (k == 'x' || k == 'X')) {
            CutSelection();
            return;
        }
        if (ctrl && (k == 12 || k == 'l' || k == 'L')) {
            OnSendForegroundPty("\x0c");
            return;
        }
        if (k == WXK_TAB && !ctrl && !e.AltDown()) {
            OnSendForegroundPty("\t");
            return;
        }
        if (k == WXK_BACK) {
            OnSendForegroundPty("\x7f");
            return;
        }
        if (k == WXK_DELETE) {
            OnSendForegroundPty("\x1b[3~");
            return;
        }
        if (k == WXK_LEFT) {
            OnSendForegroundPty("\x1b[D");
            return;
        }
        if (k == WXK_RIGHT) {
            OnSendForegroundPty("\x1b[C");
            return;
        }
        if (k == WXK_UP) {
            OnSendForegroundPty("\x1b[A");
            return;
        }
        if (k == WXK_DOWN) {
            OnSendForegroundPty("\x1b[B");
            return;
        }
        if (k == WXK_HOME) {
            OnSendForegroundPty("\x1b[H");
            return;
        }
        if (k == WXK_END) {
            OnSendForegroundPty("\x1b[F");
            return;
        }
        if (ctrl && k >= 1 && k <= 26 && k != 3 && k != 4 && k != 22 && k != 24 && k != 12) {
            const char c = static_cast<char>(k);
            OnSendForegroundPty(std::string_view(&c, 1));
            return;
        }
    }

    if (ctrl && (k == 'd' || k == 'D' || k == 4)) {
        if (m_edit.empty()) {
            if (OnEofEmptyLine)
                OnEofEmptyLine();
            return;
        }
        e.Skip();
        return;
    }

    if (k == WXK_TAB && !ctrl && !e.AltDown()) {
        if (OnTabComplete && OnTabComplete(m_edit, m_editCaret, this)) {
            m_inputAtomsDirty = true;
            m_followOutput = true;
            ScrollToFollowOutputCursor();
            Refresh();
            return;
        }
        /* No handler or not handled: ignore Tab (do not move focus). */
        return;
    }

    if (ctrl && e.ShiftDown() && (k == 'c' || k == 'C')) {
        CopySelection();
        return;
    }
    if (ctrl) {
        if (k == 'x' || k == 'X') {
            CutSelection();
            return;
        }
        if (k == 'v' || k == 'V') {
            PasteFromClipboard();
            return;
        }
        if (k == 1 || k == 'a' || k == 'A') {
            m_editCaret = 0;
            m_followOutput = true;
            ScrollToFollowOutputCursor();
            Refresh();
            return;
        }
        if (k == 5 || k == 'e' || k == 'E') {
            m_editCaret = m_edit.length();
            m_followOutput = true;
            ScrollToFollowOutputCursor();
            Refresh();
            return;
        }
        if (k == 21 || k == 'u' || k == 'U') {
            if (m_editCaret > 0) {
                m_edit.erase(0, m_editCaret);
                m_editCaret = 0;
                m_inputAtomsDirty = true;
                m_followOutput = true;
                ScrollToFollowOutputCursor();
                Refresh();
            }
            return;
        }
        if (k == 11 || k == 'k' || k == 'K') {
            if (m_editCaret < m_edit.length()) {
                m_edit.erase(m_editCaret, m_edit.length() - m_editCaret);
                m_inputAtomsDirty = true;
                m_followOutput = true;
                ScrollToFollowOutputCursor();
                Refresh();
            }
            return;
        }
        if (k == 23 || k == 'w' || k == 'W') {
            if (m_editCaret > 0) {
                size_t i = m_editCaret;
                while (i > 0 && m_edit[i - 1] == wxT(' '))
                    --i;
                while (i > 0 && m_edit[i - 1] != wxT(' '))
                    --i;
                m_edit.erase(i, m_editCaret - i);
                m_editCaret = i;
                m_inputAtomsDirty = true;
                m_followOutput = true;
                ScrollToFollowOutputCursor();
                Refresh();
            }
            return;
        }
        if (k == 12 || k == 'l' || k == 'L') {
            ClearScreen();
            return;
        }
    }

    if (k == WXK_UP && !ctrl) {
        if (m_cmdHistory.empty())
            return;
        if (m_historyNav < 0) {
            m_historyDraft = m_edit;
            m_historyNav = static_cast<int>(m_cmdHistory.size()) - 1;
        } else if (m_historyNav > 0)
            --m_historyNav;
        m_edit = m_cmdHistory[static_cast<size_t>(m_historyNav)];
        m_editCaret = m_edit.length();
        m_inputAtomsDirty = true;
        m_followOutput = true;
        ScrollToFollowOutputCursor();
        Refresh();
        return;
    }
    if (k == WXK_DOWN && !ctrl) {
        if (m_historyNav < 0)
            return;
        ++m_historyNav;
        if (m_historyNav >= static_cast<int>(m_cmdHistory.size())) {
            m_edit = m_historyDraft;
            m_historyNav = -1;
        } else
            m_edit = m_cmdHistory[static_cast<size_t>(m_historyNav)];
        m_editCaret = m_edit.length();
        m_inputAtomsDirty = true;
        m_followOutput = true;
        ScrollToFollowOutputCursor();
        Refresh();
        return;
    }

    if (k == WXK_INSERT) {
        m_insertMode = !m_insertMode;
        Refresh();
        return;
    }
    if (k == WXK_BACK) {
        if (m_editCaret > 0) {
            m_edit.erase(static_cast<size_t>(m_editCaret - 1), 1);
            --m_editCaret;
            m_inputAtomsDirty = true;
            m_followOutput = true;
            ScrollToFollowOutputCursor();
            Refresh();
        }
        return;
    }
    if (k == WXK_DELETE) {
        if (m_editCaret < m_edit.length()) {
            m_edit.erase(static_cast<size_t>(m_editCaret), 1);
            m_inputAtomsDirty = true;
            m_followOutput = true;
            ScrollToFollowOutputCursor();
            Refresh();
        }
        return;
    }
    if (k == WXK_LEFT) {
        if (m_editCaret > 0) {
            --m_editCaret;
            m_followOutput = true;
            ScrollToFollowOutputCursor();
            Refresh();
        }
        return;
    }
    if (k == WXK_RIGHT) {
        if (m_editCaret < m_edit.length()) {
            ++m_editCaret;
            m_followOutput = true;
            ScrollToFollowOutputCursor();
            Refresh();
        }
        return;
    }
    if (k == WXK_HOME) {
        m_editCaret = 0;
        m_followOutput = true;
        ScrollToFollowOutputCursor();
        Refresh();
        return;
    }
    if (k == WXK_END) {
        m_editCaret = m_edit.length();
        m_followOutput = true;
        ScrollToFollowOutputCursor();
        Refresh();
        return;
    }
    e.Skip();
}

void wxTerminal::OnSetFocus(wxFocusEvent& e) {
    m_caretOn = true;
    Refresh();
    e.Skip();
}

void wxTerminal::OnKillFocus(wxFocusEvent& e) {
    Refresh();
    e.Skip();
}

void wxTerminal::OnCaretTimer(wxTimerEvent&) {
    if (FindFocus() != this)
        return;
    m_caretOn = !m_caretOn;
    Refresh();
}

int wxTerminal::ViewportWidthPx() const {
    const int w = GetClientSize().GetWidth();
    if (w <= 0)
        return 0;
    return std::max(0, w - m_scrollbarW);
}

void wxTerminal::SyncScrollbarFromView() {
    if (!m_vscroll)
        return;
    const int maxTop = std::max(0, m_bufRows - m_visibleRows);
    const int pos = std::clamp(m_viewTop, 0, maxTop);
    const int range = std::max(1, m_bufRows);
    const int thumb = std::clamp(m_visibleRows, 1, range);
    m_vscroll->SetScrollbar(pos, thumb, range, thumb, false);
}

void wxTerminal::OnVScroll(wxScrollEvent& e) {
    if (!m_vscroll)
        return;
    const int maxTop = std::max(0, m_bufRows - m_visibleRows);
    const int pos = std::clamp(e.GetPosition(), 0, maxTop);
    m_viewTop = pos;

    const int bottom = std::max(m_cv, LastInputBufRow());
    m_followOutput = (m_viewTop + m_visibleRows - 1 >= bottom);
    Refresh();
}

void wxTerminal::HistoryPrev() {
    if (m_cmdHistory.empty())
        return;
    if (m_historyNav < 0) {
        m_historyDraft = m_edit;
        m_historyNav = static_cast<int>(m_cmdHistory.size()) - 1;
    } else if (m_historyNav > 0) {
        --m_historyNav;
    }
    m_edit = m_cmdHistory[static_cast<size_t>(m_historyNav)];
    m_editCaret = m_edit.length();
    m_inputAtomsDirty = true;
    m_followOutput = true;
    ScrollToFollowOutputCursor();
    Refresh();
}

void wxTerminal::HistoryNext() {
    if (m_historyNav < 0)
        return;
    ++m_historyNav;
    if (m_historyNav >= static_cast<int>(m_cmdHistory.size())) {
        m_edit = m_historyDraft;
        m_historyNav = -1;
    } else {
        m_edit = m_cmdHistory[static_cast<size_t>(m_historyNav)];
    }
    m_editCaret = m_edit.length();
    m_inputAtomsDirty = true;
    m_followOutput = true;
    ScrollToFollowOutputCursor();
    Refresh();
}

std::vector<std::string> wxTerminal::GetCommandHistoryUtf8() const {
    std::vector<std::string> out;
    out.reserve(m_cmdHistory.size());
    for (const auto& ws : m_cmdHistory) {
        wxScopedCharBuffer b = ws.utf8_str();
        const char* p = b.data();
        if (!p)
            continue;
        out.emplace_back(p, b.length());
    }
    return out;
}

void wxTerminal::SetCommandHistoryUtf8(const std::vector<std::string>& lines) {
    m_cmdHistory.clear();
    m_cmdHistory.reserve(lines.size());
    for (const auto& s : lines) {
        if (s.empty())
            continue;
        m_cmdHistory.emplace_back(wxString::FromUTF8(s.data(), static_cast<int>(s.size())));
    }
    m_historyNav = -1;
    m_historyDraft.clear();
}

void wxTerminal::SetFontPointSize(int pt) {
    pt = std::clamp(pt, 6, 28);
    if (pt == m_font.GetPointSize())
        return;
    m_font.SetPointSize(pt);
    SetFont(m_font);
    m_inputAtomsDirty = true;
    RecalcGrid();
}

void wxTerminal::FontSizeUp(int delta) {
    if (delta < 1)
        delta = 1;
    SetFontPointSize(m_font.GetPointSize() + delta);
}

void wxTerminal::FontSizeDown(int delta) {
    if (delta < 1)
        delta = 1;
    SetFontPointSize(m_font.GetPointSize() - delta);
}

void wxTerminal::FontSizeReset() { SetFontPointSize(m_defaultFontPt); }

void wxTerminal::PagePrev() { ScrollViewByPages(-1); }

void wxTerminal::PageNext() { ScrollViewByPages(1); }

} // namespace os

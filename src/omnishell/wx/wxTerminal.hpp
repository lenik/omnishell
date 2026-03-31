#ifndef OMNISHELL_WX_WX_TERMINAL_HPP
#define OMNISHELL_WX_WX_TERMINAL_HPP

#include "term/TermInterpreter.hpp"

#include <wx/panel.h>
#include <wx/scrolbar.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace os {

namespace term {
class XtermInterpreter;
}

/**
 * Character-grid terminal with scrollback, viewport, and input drawn at the output cursor (xterm-like).
 */
class wxTerminal : public wxPanel {
    friend class term::XtermInterpreter;

public:
    wxTerminal(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize, long style = 0);
    ~wxTerminal() override;

    // wxGTK: wxPanel often doesn't accept focus by default. Without this, we won't reliably
    // receive wxEVT_CHAR and the caret won't be shown.
    bool AcceptsFocus() const override { return true; }
    bool AcceptsFocusFromKeyboard() const override { return true; }

    /** Decode UTF-8 and feed each code point through Receive(); then Refresh(). */
    void WriteUtf8(std::string_view utf8);
    /**
     * Write UTF-8 on the first full row below the wrapped prompt+edit line, then restore m_cv/m_cu.
     * Used for tab-completion candidate lists so they do not overwrite the input line.
     */
    void WriteUtf8BelowInputOverlay(std::string_view utf8);

    void Receive(uint32_t codePoint);
    void PutChar(uint32_t codePoint);
    void PutChars(std::string_view utf8);

    void SetFgColor(const wxColour& c);
    void SetBgColor(const wxColour& c);
    void SetBold(bool on);
    void SetItalic(bool on);
    void SetInverse(bool on);
    void ResetAttributes();
    void ApplySgr(const std::vector<int>& args);

    void SetCursorVisible(bool visible);

    void GoUp(int delta = 1);
    void GoDown(int delta = 1);
    void GoLeft(int delta = 1);
    void GoRight(int delta = 1);
    void SetCursorRow(int row1Based);
    void SetCursorColumn(int col1Based);
    void SetCursorPosition(int row1Based, int col1Based);

    void EraseInDisplay(int mode = 0);
    void EraseInLine(int mode = 0);

    void ClearScreen();

    /** Convenience APIs for console UI actions. */
    void HistoryPrev();
    void HistoryNext();
    void FontSizeUp(int delta = 1);
    void FontSizeDown(int delta = 1);
    void FontSizeReset();
    void PagePrev();
    void PageNext();
    void SetFontPointSize(int pt);

    /** Persist/restore command history for consoles. */
    std::vector<std::string> GetCommandHistoryUtf8() const;
    void SetCommandHistoryUtf8(const std::vector<std::string>& lines);

    void AddInterpreter(std::unique_ptr<TermInterpreter> interp);
    void ClearInterpreters();

    /** One printable unit in the input line (prompt + edit), after parsing ANSI. */
    struct InputAtom {
        bool is_newline{false};
        wxUniChar ch{};
        /** Display width in terminal columns (1 or 2 for CJK / emoji). */
        int col_width{1};
        wxColour fg{0, 0, 0};
        wxColour bg{255, 255, 255};
        bool bold{false};
        bool italic{false};
        bool inverse{false};
    };

    void SetPrompt(const wxString& prompt);
    /** PS1 after zash expandPS1: UTF-8 with ESC sequences for colors / OSC (Kitty etc.). */
    void SetPromptUtf8(std::string_view utf8);
    wxString GetPrompt() const;

    /** Overwrite mode: block caret; insert mode: line caret. */
    void SetCaretBlockStyle(bool block) {
        m_insertMode = !block;
        Refresh();
    }
    bool GetCaretBlockStyle() const { return !m_insertMode; }

    void SetInsertMode(bool insert) {
        m_insertMode = insert;
        Refresh();
    }
    bool GetInsertMode() const { return m_insertMode; }

    std::function<void(const wxString& line)> OnSubmitLine;
    /** If set, Tab tries completion first; return true if edit/caret were updated or input consumed. */
    std::function<bool(wxString& edit, size_t& caret, wxTerminal* self)> OnTabComplete;
    /** Ctrl+D (EOF) with empty input line — e.g. close console (bash-like). */
    std::function<void()> OnEofEmptyLine;
    /** While a subprocess owns the PTY, send user input as raw terminal bytes (UTF-8). */
    std::function<void(std::string_view)> OnSendForegroundPty;
    void SetForegroundPtyPassthrough(bool on);
    bool GetForegroundPtyPassthrough() const { return m_foregroundPtyPassthrough; }

private:
    void OnPaint(wxPaintEvent& e);
    void OnSize(wxSizeEvent& e);
    void OnChar(wxKeyEvent& e);
    void OnKeyDown(wxKeyEvent& e);
    void OnVScroll(wxScrollEvent& e);
    void OnSetFocus(wxFocusEvent& e);
    void OnKillFocus(wxFocusEvent& e);
    void OnCaretTimer(wxTimerEvent& e);
    void OnMouseDown(wxMouseEvent& e);
    void OnMouseMove(wxMouseEvent& e);
    void OnMouseUp(wxMouseEvent& e);
    void OnMouseWheel(wxMouseEvent& e);

    void RecalcGrid();
    int ViewportWidthPx() const;
    void SyncScrollbarFromView();
    void ResetInterpreters();
    void EscSecond(uint8_t ch);

    void ScrollBufferUp();
    void CarriageReturn();
    void LineFeed();
    void Tab();

    enum class WidePortion : uint8_t { None = 0, Head = 1, Tail = 2 };

    struct Cell {
        wxUniChar ch = ' ';
        WidePortion wide{WidePortion::None};
        wxColour fg{0, 0, 0};
        wxColour bg{255, 255, 255};
        bool bold = false;
        bool italic = false;
        bool inverse = false;
    };

    size_t GridIndex(int bufRow, int col) const {
        return static_cast<size_t>(bufRow * m_cols + col);
    }
    Cell& BufCell(int bufRow, int col) { return m_grid[GridIndex(bufRow, col)]; }
    const Cell& BufCell(int bufRow, int col) const { return m_grid[GridIndex(bufRow, col)]; }

    int GridPixelHeight() const { return m_visibleRows * m_cellH; }
    bool HitTestVisibleCell(const wxPoint& clientPt, int* visRow, int* col) const;
    bool HitTestInputLine(const wxPoint& clientPt, int* charIndex) const;
    void ClearSelection();
    void NormalizeGridSel(int& r0, int& c0, int& r1, int& c1) const;
    void NormalizePromptSel(int& a, int& b) const;
    wxString TextFromGridSelection() const;
    wxString TextFromPromptSelection() const;
    void CopySelection();
    void CutSelection();
    void PasteFromClipboard();
    void PasteToForegroundPty();
    bool IsGridCellSelected(int visRow, int col) const;

    void ScrollViewByPages(int deltaPages);
    /** Positive = show newer content (increase m_viewTop). */
    void ScrollViewByLines(int deltaLines);
    void ScrollToFollowOutputCursor();
    int LastInputBufRow() const;
    void DrawInputLine(wxDC& dc);
    void DrawInputCaret(wxDC& dc);

    void RebuildInputLineAtoms();
    void EnsureInputAtoms() const;
    void MapFlatPosToCell(size_t pos, int* br, int* bc) const;
    wxString PromptPlainForEcho() const;

    wxColour AnsiFg(int code);
    wxColour AnsiBg(int code);

    int m_cellW{8};
    int m_cellH{16};
    int m_cols{80};
    /** Rows in the backing store (scrollback + headroom). */
    int m_bufRows{512};
    /** Rows visible in the client area (full height, no separate prompt strip). */
    int m_visibleRows{24};
    /** First buffer row shown at the top of the viewport. */
    int m_viewTop{0};
    /** When true, new output snaps the viewport to follow m_cv. */
    bool m_followOutput{true};

    std::vector<Cell> m_grid;
    int m_cu{0};
    int m_cv{0};
    bool m_cursorVisible{true};

    wxColour m_defFg{0, 0, 0};
    wxColour m_defBg{255, 255, 255};
    wxColour m_curFg{0, 0, 0};
    wxColour m_curBg{255, 255, 255};
    bool m_curBold{false};
    bool m_curItalic{false};
    bool m_curInverse{false};

    std::string m_promptUtf8;
    std::vector<InputAtom> m_inputAtoms;
    size_t m_promptAtomCount{0};
    mutable bool m_inputAtomsDirty{true};

    wxString m_edit;
    size_t m_editCaret{0};
    bool m_insertMode{true};
    bool m_caretOn{true};

    std::vector<wxString> m_cmdHistory;
    wxString m_historyDraft;
    int m_historyNav{-1};

    wxFont m_font;
    wxTimer m_caretTimer;

    std::vector<std::unique_ptr<TermInterpreter>> m_interpreters;

    bool m_foregroundPtyPassthrough{false};

    bool m_mouseDown{false};
    bool m_dragged{false};
    bool m_hasSelection{false};
    bool m_selIsGrid{true};
    int m_selGrR0{0};
    int m_selGrC0{0};
    int m_selGrR1{0};
    int m_selGrC1{0};
    int m_selPrStart{0};
    int m_selPrEnd{0};

    enum { ID_CARET_TIMER = wxID_HIGHEST + 9400 };

    wxScrollBar* m_vscroll{nullptr};
    int m_scrollbarW{12};
    int m_defaultFontPt{11};

    // Top-level key hook used to prevent scrollbar focus from breaking typing (wxGTK).
    wxWindow* m_topKeyHookWnd{nullptr};
};

} // namespace os

#endif

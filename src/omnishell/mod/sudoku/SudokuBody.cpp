#include "SudokuBody.hpp"

#include "../../ui/ThemeStyles.hpp"

#include <wx/artprov.h>
#include <wx/dcbuffer.h>
#include <wx/panel.h>
#include <wx/sizer.h>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace ThemeStyles;

namespace os {

namespace {

constexpr int N = 9;
constexpr int CELL = 48;
constexpr int PAD = 28;
constexpr int GRID_PX = PAD * 2 + CELL * N;
constexpr int TEXT_BELOW = 36;
constexpr int LOGICAL_H = GRID_PX + TEXT_BELOW;

// Classic puzzle (0 = empty); has a unique solution.
static const int kPuzzle[N][N] = {
    {5, 3, 0, 0, 7, 0, 0, 0, 0}, {6, 0, 0, 1, 9, 5, 0, 0, 0}, {0, 9, 8, 0, 0, 0, 0, 6, 0},
    {8, 0, 0, 0, 6, 0, 0, 0, 3}, {4, 0, 0, 8, 0, 3, 0, 0, 1}, {7, 0, 0, 0, 2, 0, 0, 0, 6},
    {0, 6, 0, 0, 0, 0, 2, 8, 0}, {0, 0, 0, 4, 1, 9, 0, 0, 5}, {0, 0, 0, 0, 8, 0, 0, 7, 9},
};

enum {
    ID_GROUP_GAME = uiFrame::ID_APP_HIGHEST + 410,
    ID_GAME_NEW,
    ID_GAME_UNDO,
    ID_GAME_REDO,
};

struct SudokuSnap {
    int grid[N][N]{};
    int selR{-1};
    int selC{-1};
    wxString status;
};

class SudokuCanvas : public wxPanel {
  public:
    explicit SudokuCanvas(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize(GRID_PX, LOGICAL_H));
        Bind(wxEVT_PAINT, &SudokuCanvas::OnPaint, this);
        Bind(wxEVT_SIZE, &SudokuCanvas::OnSize, this);
        Bind(wxEVT_LEFT_UP, &SudokuCanvas::OnLeftUp, this);
        Bind(wxEVT_CHAR_HOOK, &SudokuCanvas::OnCharHook, this);
        resetPuzzle();
        updateLayoutScale();
    }

    void newGame() {
        m_undo.clear();
        m_redo.clear();
        resetPuzzle();
        Refresh();
    }

    void undoMove() {
        if (m_undo.empty())
            return;
        m_redo.push_back(snap());
        apply(m_undo.back());
        m_undo.pop_back();
        Refresh();
    }

    void redoMove() {
        if (m_redo.empty())
            return;
        m_undo.push_back(snap());
        apply(m_redo.back());
        m_redo.pop_back();
        Refresh();
    }

  private:
    void resetPuzzle() {
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c) {
                m_grid[r][c] = kPuzzle[r][c];
                m_given[r][c] = (kPuzzle[r][c] != 0);
            }
        m_selR = m_selC = -1;
        m_status = "Click cell — type 1–9";
    }

    SudokuSnap snap() const {
        SudokuSnap s;
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c)
                s.grid[r][c] = m_grid[r][c];
        s.selR = m_selR;
        s.selC = m_selC;
        s.status = m_status;
        return s;
    }

    void apply(const SudokuSnap& s) {
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c)
                m_grid[r][c] = s.grid[r][c];
        m_selR = s.selR;
        m_selC = s.selC;
        m_status = s.status;
    }

    void updateLayoutScale() {
        wxSize cs = GetClientSize();
        if (cs.GetWidth() < 1 || cs.GetHeight() < 1) {
            m_scale = 1.0;
            m_offX = m_offY = 0;
            return;
        }
        double sx = static_cast<double>(cs.GetWidth()) / GRID_PX;
        double sy = static_cast<double>(cs.GetHeight()) / LOGICAL_H;
        m_scale = std::min(sx, sy);
        int tw = std::max(1, static_cast<int>(std::lround(GRID_PX * m_scale)));
        int th = std::max(1, static_cast<int>(std::lround(LOGICAL_H * m_scale)));
        m_offX = (cs.GetWidth() - tw) / 2;
        m_offY = (cs.GetHeight() - th) / 2;
    }

    void OnSize(wxSizeEvent& e) {
        updateLayoutScale();
        Refresh();
        e.Skip();
    }

    void pushHistory() {
        m_undo.push_back(snap());
        m_redo.clear();
    }

    void OnCharHook(wxKeyEvent& e) {
        int k = e.GetKeyCode();
        if (k == 'r' || k == 'R') {
            newGame();
            return;
        }
        if (k == 'h' || k == 'H') {
            m_showGridHints = !m_showGridHints;
            Refresh();
            return;
        }
        if (m_selR < 0 || m_selC < 0 || m_given[m_selR][m_selC]) {
            e.Skip();
            return;
        }
        if (k >= '1' && k <= '9') {
            pushHistory();
            m_grid[m_selR][m_selC] = k - '0';
            if (checkSolved()) {
                m_status = "Solved!";
            } else {
                m_status = "Editing…";
            }
            Refresh();
            return;
        }
        if (k == WXK_BACK || k == WXK_DELETE) {
            pushHistory();
            m_grid[m_selR][m_selC] = 0;
            m_status = "Cleared";
            Refresh();
            return;
        }
        if (k == WXK_LEFT && m_selC > 0) {
            --m_selC;
            Refresh();
            return;
        }
        if (k == WXK_RIGHT && m_selC < N - 1) {
            ++m_selC;
            Refresh();
            return;
        }
        if (k == WXK_UP && m_selR > 0) {
            --m_selR;
            Refresh();
            return;
        }
        if (k == WXK_DOWN && m_selR < N - 1) {
            ++m_selR;
            Refresh();
            return;
        }
        e.Skip();
    }

    bool screenToLogical(const wxPoint& p, double* lx, double* ly) const {
        *lx = (p.x - m_offX) / m_scale;
        *ly = (p.y - m_offY) / m_scale;
        return *lx >= 0 && *ly >= 0 && *lx < GRID_PX && *ly < GRID_PX;
    }

    void OnLeftUp(wxMouseEvent& e) {
        double lx, ly;
        if (!screenToLogical(e.GetPosition(), &lx, &ly))
            return;
        int x = static_cast<int>(lx) - PAD;
        int y = static_cast<int>(ly) - PAD;
        if (x < 0 || y < 0)
            return;
        int c = x / CELL;
        int r = y / CELL;
        if (r >= 0 && r < N && c >= 0 && c < N) {
            m_selR = r;
            m_selC = c;
            Refresh();
        }
    }

    /** Digit d (1–9) allowed in (r,c) for current grid (row / column / 3×3 box). */
    bool digitAllowedInCell(int r, int c, int d) const {
        for (int i = 0; i < N; ++i) {
            if (m_grid[r][i] == d)
                return false;
            if (m_grid[i][c] == d)
                return false;
        }
        const int br = (r / 3) * 3;
        const int bc = (c / 3) * 3;
        for (int dr = 0; dr < 3; ++dr)
            for (int dc = 0; dc < 3; ++dc)
                if (m_grid[br + dr][bc + dc] == d)
                    return false;
        return true;
    }

    void drawPencilMarks(wxDC& mdc, int cellX, int cellY, int r, int c) const {
        constexpr int inset = 2;
        const int inner = CELL - 2 * inset;
        const int sub = inner / 3;
        const int x0 = cellX + inset;
        const int y0 = cellY + inset;

        mdc.SetPen(wxPen(wxColour(210, 214, 224), 1));
        for (int i = 1; i <= 2; ++i) {
            mdc.DrawLine(x0 + i * sub, y0, x0 + i * sub, y0 + inner);
            mdc.DrawLine(x0, y0 + i * sub, x0 + inner, y0 + i * sub);
        }

        wxFont pencilFont(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        mdc.SetFont(pencilFont);
        mdc.SetTextForeground(wxColour(158, 168, 188));

        for (int d = 1; d <= 9; ++d) {
            if (!digitAllowedInCell(r, c, d))
                continue;
            const int sr = (d - 1) / 3;
            const int sc = (d - 1) % 3;
            wxString s(wxString::Format("%d", d));
            wxSize ts = mdc.GetTextExtent(s);
            const int cx = x0 + sc * sub + (sub - ts.GetWidth()) / 2;
            const int cy = y0 + sr * sub + (sub - ts.GetHeight()) / 2;
            mdc.DrawText(s, cx, cy);
        }
    }

    bool checkSolved() const {
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c)
                if (m_grid[r][c] == 0)
                    return false;
        for (int r = 0; r < N; ++r) {
            bool seen[10]{};
            for (int c = 0; c < N; ++c) {
                int v = m_grid[r][c];
                if (v < 1 || v > 9 || seen[v])
                    return false;
                seen[v] = true;
            }
        }
        for (int c = 0; c < N; ++c) {
            bool seen[10]{};
            for (int r = 0; r < N; ++r) {
                int v = m_grid[r][c];
                if (seen[v])
                    return false;
                seen[v] = true;
            }
        }
        for (int br = 0; br < 3; ++br)
            for (int bc = 0; bc < 3; ++bc) {
                bool seen[10]{};
                for (int dr = 0; dr < 3; ++dr)
                    for (int dc = 0; dc < 3; ++dc) {
                        int v = m_grid[br * 3 + dr][bc * 3 + dc];
                        if (seen[v])
                            return false;
                        seen[v] = true;
                    }
            }
        return true;
    }

    wxBitmap render() const {
        wxImage img(GRID_PX, LOGICAL_H);
        img.SetRGB(wxRect(0, 0, GRID_PX, LOGICAL_H), 245, 245, 248);

        wxBitmap bmp(img);
        wxMemoryDC mdc;
        mdc.SelectObject(bmp);
        mdc.SetBackground(wxBrush(wxColour(245, 245, 248)));
        mdc.Clear();

        mdc.SetPen(wxPen(wxColour(180, 180, 190), 1));
        for (int i = 0; i <= N; ++i) {
            int t = PAD + i * CELL;
            mdc.DrawLine(t, PAD, t, PAD + N * CELL);
            mdc.DrawLine(PAD, t, PAD + N * CELL, t);
        }
        mdc.SetPen(wxPen(wxColour(40, 40, 55), 2));
        for (int b = 0; b <= 3; ++b) {
            int t = PAD + b * 3 * CELL;
            mdc.DrawLine(t, PAD, t, PAD + N * CELL);
            mdc.DrawLine(PAD, t, PAD + N * CELL, t);
        }

        wxFont numFont(18, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                int x = PAD + c * CELL;
                int y = PAD + r * CELL;
                if (m_selR == r && m_selC == c) {
                    mdc.SetBrush(wxBrush(wxColour(200, 220, 255)));
                    mdc.SetPen(*wxTRANSPARENT_PEN);
                    mdc.DrawRectangle(x + 1, y + 1, CELL - 2, CELL - 2);
                }
                int v = m_grid[r][c];
                if (v != 0) {
                    mdc.SetFont(numFont);
                    mdc.SetTextForeground(m_given[r][c] ? wxColour(30, 30, 40)
                                                        : wxColour(20, 80, 200));
                    wxString s(wxString::Format("%d", v));
                    wxSize ts = mdc.GetTextExtent(s);
                    mdc.DrawText(s, x + (CELL - ts.GetWidth()) / 2,
                                 y + (CELL - ts.GetHeight()) / 2);
                } else if (m_showGridHints) {
                    drawPencilMarks(mdc, x, y, r, c);
                }
            }
        }

        mdc.SetTextForeground(wxColour(60, 60, 70));
        mdc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        mdc.DrawText(m_status, PAD, GRID_PX + 6);
        mdc.DrawText("R new · arrows · H hints", PAD + 200, GRID_PX + 6);

        mdc.SelectObject(wxNullBitmap);
        return bmp;
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC pdc(this);
        updateLayoutScale();
        wxBitmap bmp = render();
        wxImage img = bmp.ConvertToImage();
        int tw = std::max(1, static_cast<int>(std::lround(GRID_PX * m_scale)));
        int th = std::max(1, static_cast<int>(std::lround(LOGICAL_H * m_scale)));
        img.Rescale(tw, th, wxIMAGE_QUALITY_HIGH);
        wxBitmap scaled(img);
        pdc.DrawBitmap(scaled, m_offX, m_offY);
    }

    int m_grid[N][N]{};
    bool m_given[N][N]{};
    int m_selR{-1};
    int m_selC{-1};
    wxString m_status;
    bool m_showGridHints{false};
    std::vector<SudokuSnap> m_undo;
    std::vector<SudokuSnap> m_redo;
    double m_scale{1.0};
    int m_offX{0};
    int m_offY{0};
};

} // namespace

SudokuBody::SudokuBody(App* app) {
    const os::IconTheme* theme = (app ? app->getIconTheme() : os::app.getIconTheme());

    group(ID_GROUP_GAME, "game", "sudoku", 1000, "&Game", "Sudoku").install();
    int seq = 0;
    action(ID_GAME_NEW, "game/sudoku", "new", seq++, "&New game", "Reset puzzle")
        .icon(theme->icon("sudoku", "game.new"))
        .performFn([this](PerformContext*) {
            if (m_canvas)
                static_cast<SudokuCanvas*>(m_canvas)->newGame();
        })
        .install();
    action(ID_GAME_UNDO, "game/sudoku", "undo", seq++, "&Undo", "Undo")
        .icon(theme->icon("sudoku", "game.undo"))
        .performFn([this](PerformContext*) {
            if (m_canvas)
                static_cast<SudokuCanvas*>(m_canvas)->undoMove();
        })
        .install();
    action(ID_GAME_REDO, "game/sudoku", "redo", seq++, "&Redo", "Redo")
        .icon(theme->icon("sudoku", "game.redo"))
        .performFn([this](PerformContext*) {
            if (m_canvas)
                static_cast<SudokuCanvas*>(m_canvas)->redoMove();
        })
        .install();
}

wxWindow* SudokuBody::createFragmentView(CreateViewContext* ctx) {
    m_frame = ctx->findParentFrame();
    
    wxWindow* parent = ctx->getParent();
    m_canvas = new SudokuCanvas(parent);
    m_canvas->SetFocus();

    return m_canvas;
}

} // namespace os

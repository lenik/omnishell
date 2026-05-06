#include "FiveOrMoreBody.hpp"

#include "src/ui/ThemeStyles.hpp"

using namespace ThemeStyles;

#include <bas/ui/arch/UIElement.hpp>
#include <bas/ui/arch/UIState.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/artprov.h>
#include <wx/dcbuffer.h>
#include <wx/image.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/timer.h>

#include <algorithm>
#include <cmath>
#include <queue>
#include <random>
#include <set>
#include <variant>
#include <vector>

namespace {

constexpr int N = 9;
constexpr int CELL = 46;
constexpr int PAD = 22;
constexpr int GRID_PX = PAD * 2 + N * CELL;
constexpr int TEXT_BELOW = 42;
constexpr int LOGICAL_H = GRID_PX + TEXT_BELOW;
constexpr int NUM_COLORS = 6;
constexpr int INITIAL_BALLS_MIN = 8;
constexpr int INITIAL_BALLS_MAX = 13;
constexpr int ANIM_MS_PER_CELL = 42;

enum {
    ID_GROUP_GAME = uiFrame::ID_APP_HIGHEST + 450,
    ID_GAME_NEW,
    ID_DIFFICULTY,
};

static const wxColour kPalette[NUM_COLORS] = {
    wxColour(220, 60, 55),  // red
    wxColour(55, 120, 220), // blue
    wxColour(55, 170, 70),  // green
    wxColour(230, 190, 45), // yellow
    wxColour(160, 70, 200), // purple
    wxColour(240, 120, 40), // orange
};

class FiveOrMoreCanvas : public wxPanel {
  public:
    explicit FiveOrMoreCanvas(wxWindow* parent, os::FiveOrMoreBody* body)
        : wxPanel(parent, wxID_ANY), m_body(body) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize(GRID_PX, LOGICAL_H));
        Bind(wxEVT_PAINT, &FiveOrMoreCanvas::OnPaint, this);
        Bind(wxEVT_SIZE, &FiveOrMoreCanvas::OnSize, this);
        Bind(wxEVT_LEFT_UP, &FiveOrMoreCanvas::OnLeftUp, this);
        Bind(wxEVT_CHAR_HOOK, &FiveOrMoreCanvas::OnCharHook, this);
        m_animTimer.SetOwner(this);
        Bind(wxEVT_TIMER, &FiveOrMoreCanvas::OnAnimTimer, this, m_animTimer.GetId());
        newGame();
        updateLayoutScale();
    }

    void newGame() {
        if (m_body)
            m_body->canvasNotifyNewGame();
        m_spawnPerMove = m_body ? m_body->chosenSpawnPerMove() : 3;
        if (m_spawnPerMove < 1)
            m_spawnPerMove = 1;
        if (m_spawnPerMove > 3)
            m_spawnPerMove = 3;

        m_animTimer.Stop();
        m_animPath.clear();
        m_animStep = 0;
        m_animColor = 0;
        m_grid.assign(N * N, 0);
        m_score = 0;
        m_gameOver = false;
        m_selR = m_selC = -1;
        m_status = wxString::Format(
            wxT("Score 0 - %d ball(s)/move - select a ball, then an empty cell"), m_spawnPerMove);

        std::random_device rd;
        std::mt19937 g(rd());
        std::uniform_int_distribution<int> cntDist(INITIAL_BALLS_MIN, INITIAL_BALLS_MAX);
        std::uniform_int_distribution<int> posDist(0, N * N - 1);
        std::uniform_int_distribution<int> colDist(1, NUM_COLORS);
        int n = cntDist(g);
        for (int i = 0; i < n; ++i) {
            for (int tries = 0; tries < 80; ++tries) {
                int p = posDist(g);
                if (m_grid[p] == 0) {
                    m_grid[p] = colDist(g);
                    break;
                }
            }
        }
        clearLinesLoop();
        Refresh();
    }

  private:
    int idx(int r, int c) const { return r * N + c; }

    bool inBounds(int r, int c) const { return r >= 0 && r < N && c >= 0 && c < N; }

    int countEmpty() const {
        int n = 0;
        for (int v : m_grid)
            if (v == 0)
                ++n;
        return n;
    }

    bool findPath(int sr, int sc, int dr, int dc, std::vector<std::pair<int, int>>& outPath) const {
        outPath.clear();
        if (!inBounds(sr, sc) || !inBounds(dr, dc))
            return false;
        if (m_grid[idx(sr, sc)] == 0 || m_grid[idx(dr, dc)] != 0)
            return false;

        const int start = idx(sr, sc);
        const int goal = idx(dr, dc);
        std::queue<std::pair<int, int>> q;
        std::vector<int> parent(N * N, -1);
        q.push({sr, sc});
        parent[start] = start;

        const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        while (!q.empty()) {
            auto [r, c] = q.front();
            q.pop();
            const int u = idx(r, c);
            if (u == goal) {
                std::vector<std::pair<int, int>> rev;
                int cur = goal;
                while (cur != start) {
                    rev.push_back({cur / N, cur % N});
                    cur = parent[cur];
                }
                rev.push_back({sr, sc});
                std::reverse(rev.begin(), rev.end());
                outPath = std::move(rev);
                return true;
            }
            for (auto& d : dirs) {
                int nr = r + d[0];
                int nc = c + d[1];
                if (!inBounds(nr, nc))
                    continue;
                const int v = idx(nr, nc);
                if (parent[v] >= 0)
                    continue;
                if (nr == dr && nc == dc) {
                    parent[v] = u;
                    q.push({nr, nc});
                    continue;
                }
                if (m_grid[v] != 0)
                    continue;
                parent[v] = u;
                q.push({nr, nc});
            }
        }
        return false;
    }

    std::set<std::pair<int, int>> findLineRemovals() const {
        std::set<std::pair<int, int>> toRemove;
        const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                int col = m_grid[idx(r, c)];
                if (col == 0)
                    continue;
                for (auto& d : dirs) {
                    int dr = d[0], dc = d[1];
                    std::vector<std::pair<int, int>> seg;
                    seg.push_back({r, c});
                    int r2 = r + dr, c2 = c + dc;
                    while (inBounds(r2, c2) && m_grid[idx(r2, c2)] == col) {
                        seg.push_back({r2, c2});
                        r2 += dr;
                        c2 += dc;
                    }
                    r2 = r - dr;
                    c2 = c - dc;
                    while (inBounds(r2, c2) && m_grid[idx(r2, c2)] == col) {
                        seg.push_back({r2, c2});
                        r2 -= dr;
                        c2 -= dc;
                    }
                    if (seg.size() >= 5) {
                        for (auto& p : seg)
                            toRemove.insert(p);
                    }
                }
            }
        }
        return toRemove;
    }

    void clearLinesLoop() {
        for (;;) {
            auto rm = findLineRemovals();
            if (rm.empty())
                break;
            for (auto& p : rm)
                m_grid[idx(p.first, p.second)] = 0;
            m_score += static_cast<int>(rm.size()) * 10;
        }
    }

    void spawnRandomBalls(int n) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::uniform_int_distribution<int> posDist(0, N * N - 1);
        std::uniform_int_distribution<int> colDist(1, NUM_COLORS);
        int placed = 0;
        for (int i = 0; i < n * 120 && placed < n; ++i) {
            int p = posDist(g);
            if (m_grid[p] == 0) {
                m_grid[p] = colDist(g);
                ++placed;
            }
        }
    }

    void afterMove() {
        if (m_body)
            m_body->canvasNotifyMoveFinished();

        clearLinesLoop();
        if (countEmpty() < m_spawnPerMove) {
            m_gameOver = true;
            m_status =
                wxString::Format(wxT("No space for new balls - Score %d. R new game"), m_score);
            return;
        }
        spawnRandomBalls(m_spawnPerMove);
        m_status = wxString::Format(wxT("Score %d - move again"), m_score);
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

    void OnCharHook(wxKeyEvent& e) {
        if (e.GetKeyCode() == 'r' || e.GetKeyCode() == 'R') {
            newGame();
            return;
        }
        e.Skip();
    }

    void OnAnimTimer(wxTimerEvent&) {
        if (m_animPath.empty())
            return;
        ++m_animStep;
        if (m_animStep < static_cast<int>(m_animPath.size())) {
            Refresh();
            return;
        }
        const int dr = m_animPath.back().first;
        const int dc = m_animPath.back().second;
        m_grid[idx(dr, dc)] = m_animColor;
        m_animTimer.Stop();
        m_animPath.clear();
        m_animStep = 0;
        m_animColor = 0;
        afterMove();
        Refresh();
    }

    bool screenToLogical(const wxPoint& p, double* lx, double* ly) const {
        *lx = (p.x - m_offX) / m_scale;
        *ly = (p.y - m_offY) / m_scale;
        return *lx >= 0 && *ly >= 0 && *lx < GRID_PX && *ly < GRID_PX;
    }

    void OnLeftUp(wxMouseEvent& e) {
        if (!m_animPath.empty())
            return;
        if (m_gameOver) {
            Refresh();
            return;
        }
        double lx, ly;
        if (!screenToLogical(e.GetPosition(), &lx, &ly))
            return;
        int x = static_cast<int>(lx) - PAD;
        int y = static_cast<int>(ly) - PAD;
        if (x < 0 || y < 0)
            return;
        int c = x / CELL;
        int r = y / CELL;
        if (r < 0 || r >= N || c < 0 || c >= N)
            return;

        if (m_grid[idx(r, c)] != 0) {
            if (m_selR == r && m_selC == c)
                m_selR = m_selC = -1;
            else {
                m_selR = r;
                m_selC = c;
            }
            Refresh();
            return;
        }

        if (m_selR < 0 || m_selC < 0) {
            Refresh();
            return;
        }

        std::vector<std::pair<int, int>> path;
        if (!findPath(m_selR, m_selC, r, c, path) || path.size() < 2) {
            Refresh();
            return;
        }

        m_animColor = m_grid[idx(m_selR, m_selC)];
        m_grid[idx(m_selR, m_selC)] = 0;
        m_animPath = std::move(path);
        m_animStep = 0;
        m_selR = m_selC = -1;
        m_animTimer.Start(ANIM_MS_PER_CELL);
        Refresh();
    }

    wxBitmap render() const {
        wxImage img(GRID_PX, LOGICAL_H);
        img.SetRGB(wxRect(0, 0, GRID_PX, LOGICAL_H), 248, 248, 252);

        wxBitmap bmp(img);
        wxMemoryDC mdc;
        mdc.SelectObject(bmp);
        mdc.SetBackground(wxBrush(wxColour(248, 248, 252)));
        mdc.Clear();

        mdc.SetPen(wxPen(wxColour(200, 200, 210), 1));
        for (int i = 0; i <= N; ++i) {
            int t = PAD + i * CELL;
            mdc.DrawLine(t, PAD, t, PAD + N * CELL);
            mdc.DrawLine(PAD, t, PAD + N * CELL, t);
        }

        const int rad = static_cast<int>(CELL * 0.36);
        auto drawBall = [&](int cx, int cy, int colorIdx) {
            if (colorIdx < 1 || colorIdx > NUM_COLORS)
                return;
            wxColour col = kPalette[colorIdx - 1];
            mdc.SetBrush(wxBrush(col));
            mdc.SetPen(wxPen(wxColour(std::max(0, col.Red() - 45), std::max(0, col.Green() - 45),
                                      std::max(0, col.Blue() - 45)),
                             1));
            mdc.DrawEllipse(cx - rad, cy - rad, rad * 2, rad * 2);
        };

        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                int v = m_grid[idx(r, c)];
                if (v < 1 || v > NUM_COLORS)
                    continue;
                int cx = PAD + c * CELL + CELL / 2;
                int cy = PAD + r * CELL + CELL / 2;
                drawBall(cx, cy, v);
            }
        }

        if (!m_animPath.empty() && m_animStep >= 0 &&
            m_animStep < static_cast<int>(m_animPath.size()) && m_animColor >= 1) {
            const auto& cell = m_animPath[static_cast<size_t>(m_animStep)];
            int cx = PAD + cell.second * CELL + CELL / 2;
            int cy = PAD + cell.first * CELL + CELL / 2;
            drawBall(cx, cy, m_animColor);
        }

        if (m_selR >= 0 && m_selC >= 0) {
            int x = PAD + m_selC * CELL;
            int y = PAD + m_selR * CELL;
            mdc.SetPen(wxPen(wxColour(40, 80, 200), 3));
            mdc.SetBrush(*wxTRANSPARENT_BRUSH);
            mdc.DrawRectangle(x + 2, y + 2, CELL - 4, CELL - 4);
        }

        mdc.SetTextForeground(wxColour(50, 50, 60));
        mdc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        mdc.DrawText(m_status, PAD, GRID_PX + 8);
        mdc.DrawText(wxT("R new - change level before first move"), PAD + 260, GRID_PX + 8);

        mdc.SelectObject(wxNullBitmap);
        return bmp;
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC pdc(this);
        updateLayoutScale();
        wxBitmap bmp = render();
        wxImage simg = bmp.ConvertToImage();
        int tw = std::max(1, static_cast<int>(std::lround(GRID_PX * m_scale)));
        int th = std::max(1, static_cast<int>(std::lround(LOGICAL_H * m_scale)));
        simg.Rescale(tw, th, wxIMAGE_QUALITY_HIGH);
        wxBitmap scaled(simg);
        pdc.DrawBitmap(scaled, m_offX, m_offY);
    }

    os::FiveOrMoreBody* m_body{nullptr};
    int m_spawnPerMove{3};

    std::vector<int> m_grid;
    int m_score{0};
    int m_selR{-1};
    int m_selC{-1};
    wxString m_status;
    bool m_gameOver{false};

    wxTimer m_animTimer;
    std::vector<std::pair<int, int>> m_animPath;
    int m_animStep{0};
    int m_animColor{0};

    double m_scale{1.0};
    int m_offX{0};
    int m_offY{0};
};

} // namespace

namespace os {

void FiveOrMoreBody::syncDifficultyToolbar(bool enable) {
    for (UIElement* el : elements()) {
        if (el->id == ID_DIFFICULTY) {
            el->enabled.set(enable);
            return;
        }
    }
}

void FiveOrMoreBody::canvasNotifyNewGame() {
    m_movesCompleted = 0;
    syncDifficultyToolbar(true);
}

void FiveOrMoreBody::canvasNotifyMoveFinished() {
    m_movesCompleted++;
    if (m_movesCompleted > 0)
        syncDifficultyToolbar(false);
}

FiveOrMoreBody::FiveOrMoreBody(App* app) {
    const os::IconTheme* theme = (app ? app->getIconTheme() : os::app.getIconTheme());

    group(ID_GROUP_GAME, "game", "fiveormore", 1000, "&Game", "Five or more").install();

    observable<UIStateVariant>* difficultyValueSink = nullptr;

    state(ID_DIFFICULTY, "game/fiveormore", "difficulty", 5)
        .label("Difficulty")
        .description("Balls per move after each turn; start a new game to apply.")
        .stateType(UIStateType::ENUM)
        .enumValues({1, 2, 3})
        .initValue(UIStateVariant{3})
        .valueRef(&difficultyValueSink)
        .valueDescriptorFn([](int v) -> UIStateValueDescriptor {
            UIStateValueDescriptor d;
            d.label = v == 1 ? "Easy" : (v == 2 ? "Medium" : "Hard");
            d.description = v == 1 ? "1 new ball per move"
                                   : (v == 2 ? "2 new balls per move" : "3 new balls per move");
            if (v == 1)
                d.icon = ImageSet(Path(slv_core_flat, "phone/signal-low.svg"));
            else if (v == 2)
                d.icon = ImageSet(Path(slv_core_flat, "phone/signal-medium.svg"));
            else
                d.icon = ImageSet(Path(slv_core_flat, "phone/signal-full.svg"));
            return d;
        })
        .connect([this](const UIStateVariant& nv, const UIStateVariant&) {
            if (const int* p = std::get_if<int>(&nv))
                m_chosenLevel = *p;
        })
        .install();

    action(ID_GAME_NEW, "game/fiveormore", "new", 0, "&New game", "New board")
        .icon(theme->icon("fiveormore", "game.new"))
        .performFn([this](PerformContext*) {
            if (m_canvas)
                static_cast<FiveOrMoreCanvas*>(m_canvas)->newGame();
        })
        .install();
}

void FiveOrMoreBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    if (!dynamic_cast<uiFrame*>(parent))
        return;

    auto* panel = new FiveOrMoreCanvas(parent, this);
    m_canvas = panel;
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(panel, 1, wxEXPAND);
    parent->SetSizer(s);
    panel->SetFocus();
}

} // namespace os

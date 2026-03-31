#include "SolitaireBody.hpp"

#include "../../ui/ThemeStyles.hpp"

#include <wx/artprov.h>
#include <wx/dcbuffer.h>
#include <wx/image.h>
#include <wx/panel.h>
#include <wx/sizer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <vector>

using namespace ThemeStyles;

namespace os {

namespace {

constexpr int CW = 56;
constexpr int CH = 78;
constexpr int PAD = 14;
constexpr int COL_GAP = 10;
constexpr int TABLEAU_OVERLAP = 18;
constexpr int W = 900;
constexpr int H = 720;

constexpr int STOCK_X = PAD;
constexpr int STOCK_Y = PAD;
constexpr int WASTE_X = STOCK_X + CW + 14;
constexpr int FOUNDATION_X0 = WASTE_X + CW + 36;
constexpr int TABLEAU_TOP = PAD + CH + 32;

enum {
    ID_GROUP_GAME = uiFrame::ID_APP_HIGHEST + 440,
    ID_GAME_NEW,
};

struct Card {
    int rank{1}; // 1=A .. 13=K
    int suit{0}; // 0..3
    bool faceUp{false};
};

static bool isRed(const Card& c) { return c.suit == 2 || c.suit == 3; }

static wxString rankStr(int r) {
    if (r == 1)
        return "A";
    if (r == 11)
        return "J";
    if (r == 12)
        return "Q";
    if (r == 13)
        return "K";
    return wxString::Format("%d", r);
}

static wxString suitStr(int s) {
    // clubs hearts diamonds spades
    // static const char* t[] = {"C", "H", "D", "S"};
    static wxString t[] = // {"♣", "♥", "♦", "♠"};
        {
            wxString::FromUTF8("\xE2\x99\xA3"), // Clubs
            wxString::FromUTF8("\xE2\x99\xA5"), // Hearts
            wxString::FromUTF8("\xE2\x99\xA6"), // Diamonds
            wxString::FromUTF8("\xE2\x99\xA0"), // Spades
        };
    return t[s & 3];
}

static wxColour suitColour(int s) {
    return (s == 2 || s == 3) ? wxColour(200, 35, 35) //
                              : wxColour(25, 25, 35);
}

static int tabX(int col) { return PAD + col * (CW + COL_GAP); }

static int tableauBottomY(const std::vector<Card>& pile) {
    if (pile.empty())
        return TABLEAU_TOP;
    return TABLEAU_TOP + static_cast<int>((pile.size() - 1) * TABLEAU_OVERLAP) + CH;
}

/** Bottom of run at `from` through top: face-up, descending rank, alternating color. */
static bool validTableauRun(const std::vector<Card>& pile, size_t from) {
    if (from >= pile.size())
        return false;
    for (size_t k = from; k < pile.size(); ++k) {
        if (!pile[k].faceUp)
            return false;
    }
    for (size_t k = from; k + 1 < pile.size(); ++k) {
        if (pile[k].rank != pile[k + 1].rank + 1)
            return false;
        if (isRed(pile[k]) == isRed(pile[k + 1]))
            return false;
    }
    return true;
}

class SolitaireCanvas : public wxPanel {
  public:
    explicit SolitaireCanvas(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize(W, H));
        Bind(wxEVT_PAINT, &SolitaireCanvas::OnPaint, this);
        Bind(wxEVT_SIZE, &SolitaireCanvas::OnSize, this);
        Bind(wxEVT_LEFT_UP, &SolitaireCanvas::OnLeftUp, this);
        Bind(wxEVT_CHAR_HOOK, &SolitaireCanvas::OnCharHook, this);
        newDeal();
        updateLayoutScale();
    }

    void newGame() {
        clearSelection();
        newDeal();
        Refresh();
    }

  private:
    void clearSelection() {
        m_selWaste = false;
        m_selTabCol = -1;
        m_selTabFrom = 0;
    }

    void newDeal() {
        std::vector<Card> deck;
        deck.reserve(52);
        for (int s = 0; s < 4; ++s)
            for (int r = 1; r <= 13; ++r)
                deck.push_back(Card{r, s, false});
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(deck.begin(), deck.end(), g);

        m_stock.clear();
        m_waste.clear();
        for (auto& t : m_tableau)
            t.clear();
        for (auto& f : m_foundation)
            f.clear();

        size_t idx = 0;
        for (int c = 0; c < 7; ++c) {
            for (int r = 0; r < c; ++r)
                m_tableau[c].push_back(deck[idx++]);
            m_tableau[c].push_back(deck[idx++]);
            m_tableau[c].back().faceUp = true;
        }
        while (idx < deck.size())
            m_stock.push_back(deck[idx++]);

        m_won = false;
        m_status = "Draw from stock · build tableau · foundations A→K";
        clearSelection();
    }

    bool checkWin() const {
        for (const auto& f : m_foundation) {
            if (f.size() != 13)
                return false;
        }
        return true;
    }

    void updateLayoutScale() {
        wxSize cs = GetClientSize();
        if (cs.GetWidth() < 1 || cs.GetHeight() < 1) {
            m_scale = 1.0;
            m_offX = m_offY = 0;
            return;
        }
        double sx = static_cast<double>(cs.GetWidth()) / W;
        double sy = static_cast<double>(cs.GetHeight()) / H;
        m_scale = std::min(sx, sy);
        int tw = std::max(1, static_cast<int>(std::lround(W * m_scale)));
        int th = std::max(1, static_cast<int>(std::lround(H * m_scale)));
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

    bool screenToLogical(const wxPoint& p, double* lx, double* ly) const {
        *lx = (p.x - m_offX) / m_scale;
        *ly = (p.y - m_offY) / m_scale;
        return *lx >= 0 && *ly >= 0 && *lx < W && *ly < H;
    }

    bool hitStock(int lx, int ly) const {
        return lx >= STOCK_X && lx < STOCK_X + CW && ly >= STOCK_Y && ly < STOCK_Y + CH;
    }

    bool hitWaste(int lx, int ly) const {
        return lx >= WASTE_X && lx < WASTE_X + CW && ly >= STOCK_Y && ly < STOCK_Y + CH;
    }

    bool hitFoundation(int fi, int lx, int ly) const {
        int x = FOUNDATION_X0 + fi * (CW + COL_GAP);
        return lx >= x && lx < x + CW && ly >= STOCK_Y && ly < STOCK_Y + CH;
    }

    /** Returns pile index or -1. */
    int hitTableauColumn(int lx, int ly) const {
        for (int c = 0; c < 7; ++c) {
            int x0 = tabX(c);
            if (lx < x0 || lx >= x0 + CW)
                continue;
            const auto& pile = m_tableau[c];
            if (pile.empty()) {
                if (ly >= TABLEAU_TOP && ly < TABLEAU_TOP + CH)
                    return c;
                continue;
            }
            int bottom = tableauBottomY(pile);
            if (ly < TABLEAU_TOP || ly >= bottom)
                continue;
            return c;
        }
        return -1;
    }

    int tableauCardAt(int col, int ly) const {
        const auto& pile = m_tableau[col];
        if (pile.empty())
            return -1;
        for (int k = static_cast<int>(pile.size()) - 1; k >= 0; --k) {
            int yk = TABLEAU_TOP + k * TABLEAU_OVERLAP;
            if (ly >= yk && ly < yk + CH)
                return k;
        }
        return -1;
    }

    bool canPlaceOnFoundation(const Card& c, int fi) const {
        const auto& f = m_foundation[fi];
        if (f.empty())
            return c.rank == 1;
        const Card& t = f.back();
        return t.suit == c.suit && c.rank == t.rank + 1;
    }

    bool canPlaceRunOnTableau(const std::vector<Card>& run, int destCol) const {
        if (run.empty())
            return false;
        const Card& bottom = run.front();
        auto& dest = m_tableau[destCol];
        if (dest.empty())
            return bottom.rank == 13;
        const Card& top = dest.back();
        if (!top.faceUp)
            return false;
        return top.rank == bottom.rank + 1 && isRed(top) != isRed(bottom);
    }

    void drawStockOrRecycle() {
        if (!m_stock.empty()) {
            Card c = m_stock.back();
            m_stock.pop_back();
            c.faceUp = true;
            m_waste.push_back(c);
            return;
        }
        // Turn waste into face-down stock: next draw from stock is the card that was under
        // the waste top; old waste top is drawn last before the next recycle.
        if (!m_waste.empty()) {
            m_stock.clear();
            for (int i = static_cast<int>(m_waste.size()) - 2; i >= 0; --i) {
                Card c = m_waste[static_cast<size_t>(i)];
                c.faceUp = false;
                m_stock.push_back(c);
            }
            Card top = m_waste.back();
            top.faceUp = false;
            m_stock.insert(m_stock.begin(), top);
            m_waste.clear();
        }
    }

    void tryMoveToFoundation(int fi) {
        if (m_selWaste) {
            if (m_waste.empty())
                return;
            Card c = m_waste.back();
            if (!canPlaceOnFoundation(c, fi))
                return;
            m_waste.pop_back();
            m_foundation[fi].push_back(c);
            clearSelection();
            if (checkWin()) {
                m_won = true;
                m_status = "You win! R for new deal";
            }
            return;
        }
        if (m_selTabCol >= 0) {
            auto& src = m_tableau[m_selTabCol];
            if (m_selTabFrom != static_cast<int>(src.size()) - 1)
                return;
            Card c = src.back();
            if (!canPlaceOnFoundation(c, fi))
                return;
            src.pop_back();
            m_foundation[fi].push_back(c);
            if (!src.empty() && !src.back().faceUp)
                src.back().faceUp = true;
            clearSelection();
            if (checkWin()) {
                m_won = true;
                m_status = "You win! R for new deal";
            }
        }
    }

    void tryMoveToTableau(int destCol) {
        if (m_selTabCol == destCol)
            return;
        std::vector<Card> run;
        if (m_selWaste) {
            if (m_waste.empty())
                return;
            run.push_back(m_waste.back());
        } else if (m_selTabCol >= 0) {
            auto& src = m_tableau[m_selTabCol];
            if (m_selTabFrom < 0 || m_selTabFrom >= static_cast<int>(src.size()))
                return;
            run.assign(src.begin() + m_selTabFrom, src.end());
        } else {
            return;
        }

        if (!canPlaceRunOnTableau(run, destCol))
            return;

        if (m_selWaste) {
            m_waste.pop_back();
        } else {
            auto& src = m_tableau[m_selTabCol];
            src.erase(src.begin() + m_selTabFrom, src.end());
            if (!src.empty() && !src.back().faceUp)
                src.back().faceUp = true;
        }
        auto& dest = m_tableau[destCol];
        dest.insert(dest.end(), run.begin(), run.end());
        clearSelection();
    }

    void OnLeftUp(wxMouseEvent& e) {
        if (m_won) {
            Refresh();
            return;
        }

        double lx, ly;
        if (!screenToLogical(e.GetPosition(), &lx, &ly))
            return;
        const int x = static_cast<int>(lx);
        const int y = static_cast<int>(ly);

        if (hitStock(x, y)) {
            clearSelection();
            drawStockOrRecycle();
            Refresh();
            return;
        }

        for (int fi = 0; fi < 4; ++fi) {
            if (hitFoundation(fi, x, y)) {
                if (m_selWaste || m_selTabCol >= 0)
                    tryMoveToFoundation(fi);
                Refresh();
                return;
            }
        }

        if (hitWaste(x, y)) {
            if (!m_waste.empty()) {
                if (m_selWaste)
                    clearSelection();
                else {
                    clearSelection();
                    m_selWaste = true;
                }
            }
            Refresh();
            return;
        }

        int tc = hitTableauColumn(x, y);
        if (tc >= 0) {
            int cardIdx = tableauCardAt(tc, y);
            auto& pile = m_tableau[tc];

            if (cardIdx < 0 && pile.empty()) {
                if (m_selWaste || m_selTabCol >= 0)
                    tryMoveToTableau(tc);
                Refresh();
                return;
            }

            if (cardIdx >= 0 && !pile[cardIdx].faceUp) {
                if (cardIdx == static_cast<int>(pile.size()) - 1) {
                    pile[cardIdx].faceUp = true;
                    clearSelection();
                }
                Refresh();
                return;
            }

            if (cardIdx >= 0) {
                if ((m_selWaste || m_selTabCol >= 0) && (m_selTabCol != tc || m_selWaste)) {
                    tryMoveToTableau(tc);
                    Refresh();
                    return;
                }
                if (validTableauRun(pile, static_cast<size_t>(cardIdx))) {
                    if (m_selTabCol == tc && m_selTabFrom == cardIdx)
                        clearSelection();
                    else {
                        m_selWaste = false;
                        m_selTabCol = tc;
                        m_selTabFrom = cardIdx;
                    }
                }
            }
            Refresh();
            return;
        }

        clearSelection();
        Refresh();
    }

    static void drawCardBack(wxDC& dc, int x, int y) {
        dc.SetPen(wxPen(wxColour(30, 50, 100), 1));
        dc.SetBrush(wxBrush(wxColour(40, 70, 150)));
        dc.DrawRoundedRectangle(x, y, CW, CH, 5);
        dc.SetPen(wxPen(wxColour(90, 120, 190), 1));
        dc.DrawRoundedRectangle(x + 4, y + 4, CW - 8, CH - 8, 3);
    }

    static void drawCardFace(wxDC& dc, int x, int y, const Card& c) {
        dc.SetPen(wxPen(wxColour(40, 40, 50), 1));
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRoundedRectangle(x, y, CW, CH, 5);
        dc.SetFont(wxFont(11, wxFONTFAMILY_DEFAULT,              //
                          wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, //
                          false));
        dc.SetTextForeground(suitColour(c.suit));
        wxString line = rankStr(c.rank) + suitStr(c.suit);

        dc.DrawText(line, x + 6, y + 8);
    }

    void drawOutline(wxDC& dc, int x, int y) const {
        dc.SetPen(wxPen(wxColour(255, 210, 60), 3));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRoundedRectangle(x - 1, y - 1, CW + 2, CH + 2, 6);
    }

    wxBitmap render() const {
        wxImage img(W, H);
        img.SetRGB(wxRect(0, 0, W, H), 18, 110, 72);

        wxBitmap bmp(img);
        wxMemoryDC mdc;
        mdc.SelectObject(bmp);
        mdc.SetBackground(wxBrush(wxColour(18, 110, 72)));
        mdc.Clear();

        if (m_stock.empty())
            mdc.SetBrush(wxBrush(wxColour(30, 90, 50)));
        else
            mdc.SetBrush(wxBrush(wxColour(40, 120, 65)));
        mdc.SetPen(wxPen(wxColour(60, 140, 80), 1));
        mdc.DrawRoundedRectangle(STOCK_X, STOCK_Y, CW, CH, 5);
        mdc.SetTextForeground(wxColour(200, 230, 210));
        mdc.SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        mdc.DrawText(m_stock.empty() && !m_waste.empty() ? "Redeal" : "Stock", STOCK_X + 8,
                     STOCK_Y + CH / 2 - 6);

        mdc.SetPen(wxPen(wxColour(50, 50, 55), 1));
        mdc.SetBrush(*wxTRANSPARENT_BRUSH);
        mdc.DrawRectangle(WASTE_X, STOCK_Y, CW, CH);
        if (!m_waste.empty()) {
            const Card& w = m_waste.back();
            drawCardFace(mdc, WASTE_X, STOCK_Y, w);
            if (m_selWaste)
                drawOutline(mdc, WASTE_X, STOCK_Y);
        }

        for (int fi = 0; fi < 4; ++fi) {
            int fx = FOUNDATION_X0 + fi * (CW + COL_GAP);
            // drawCardFace leaves wxWHITE_BRUSH set; DrawRectangle would fill empty slots solid
            // white.
            mdc.SetPen(wxPen(wxColour(50, 50, 55), 1));
            mdc.SetBrush(*wxTRANSPARENT_BRUSH);
            mdc.DrawRectangle(fx, STOCK_Y, CW, CH);
            if (!m_foundation[fi].empty())
                drawCardFace(mdc, fx, STOCK_Y, m_foundation[fi].back());
        }

        for (int c = 0; c < 7; ++c) {
            int x0 = tabX(c);
            const auto& pile = m_tableau[c];
            for (size_t k = 0; k < pile.size(); ++k) {
                int yk = TABLEAU_TOP + static_cast<int>(k * TABLEAU_OVERLAP);
                if (pile[k].faceUp)
                    drawCardFace(mdc, x0, yk, pile[k]);
                else
                    drawCardBack(mdc, x0, yk);
                if (m_selTabCol == c && m_selTabFrom >= 0 && static_cast<int>(k) >= m_selTabFrom)
                    drawOutline(mdc, x0, yk);
            }
        }

        mdc.SetTextForeground(wxColour(220, 240, 225));
        mdc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        mdc.DrawText(m_status, PAD, H - 26);
        mdc.DrawText("R new deal", W - 100, H - 26);

        mdc.SelectObject(wxNullBitmap);
        return bmp;
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC pdc(this);
        updateLayoutScale();
        wxBitmap bmp = render();
        wxImage simg = bmp.ConvertToImage();
        int tw = std::max(1, static_cast<int>(std::lround(W * m_scale)));
        int th = std::max(1, static_cast<int>(std::lround(H * m_scale)));
        simg.Rescale(tw, th, wxIMAGE_QUALITY_HIGH);
        wxBitmap scaled(simg);
        pdc.DrawBitmap(scaled, m_offX, m_offY);
    }

    std::vector<Card> m_stock;
    std::vector<Card> m_waste;
    std::array<std::vector<Card>, 4> m_foundation;
    std::array<std::vector<Card>, 7> m_tableau;

    bool m_selWaste{false};
    int m_selTabCol{-1};
    int m_selTabFrom{0};

    wxString m_status;
    bool m_won{false};

    double m_scale{1.0};
    int m_offX{0};
    int m_offY{0};
};

} // namespace

SolitaireBody::SolitaireBody(App* app) {
    const os::IconTheme* theme = (app ? app->getIconTheme() : os::app.getIconTheme());

    group(ID_GROUP_GAME, "game", "solitaire", 1000)
        .label("&Game")
        .description("Solitaire")
        .install();
    action(ID_GAME_NEW, "game/solitaire", "new", 0)
        .label("&New deal")
        .description("Shuffle and deal")
        .icon(theme->icon("solitaire", "game.new"))
        .performFn([this](PerformContext*) {
            if (m_canvas)
                static_cast<SolitaireCanvas*>(m_canvas)->newGame();
        })
        .install();
}

void SolitaireBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    auto* panel = new SolitaireCanvas(parent);
    m_canvas = panel;
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(panel, 1, wxEXPAND);
    parent->SetSizer(s);
    panel->SetFocus();
}

wxEvtHandler* SolitaireBody::getEventHandler() {
    return m_canvas ? m_canvas->GetEventHandler() : nullptr;
}

} // namespace os

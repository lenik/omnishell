#include "MinesweeperBody.hpp"

#include "../../core/App.hpp"
#include "../../ui/ThemeStyles.hpp"

#include <wx/artprov.h>

#include <algorithm>
#include <random>

using namespace ThemeStyles;

namespace os {

namespace {
enum {
    ID_GROUP_GAME = uiFrame::ID_APP_HIGHEST + 430,
    ID_GAME_NEW,
    ID_GAME_UNDO,
    ID_GAME_REDO,
};
}

MinesweeperBody::MinesweeperBody(App* app) {
    const os::IconTheme* theme = (app ? app->getIconTheme() : os::app.getIconTheme());

    group(ID_GROUP_GAME, "game", "minesweeper", 1000, "&Game", "Minesweeper").install();
    int seq = 0;
    action(ID_GAME_NEW, "game/minesweeper", "new", seq++, "&New game", "New game")
        .icon(theme->icon("minesweeper", "game.new"))
        .performFn([this](PerformContext*) { resetGame(); })
        .install();
    action(ID_GAME_UNDO, "game/minesweeper", "undo", seq++, "&Undo", "Undo")
        .icon(theme->icon("minesweeper", "game.undo"))
        .performFn([this](PerformContext*) { undoMove(); })
        .install();
    action(ID_GAME_REDO, "game/minesweeper", "redo", seq++, "&Redo", "Redo")
        .icon(theme->icon("minesweeper", "game.redo"))
        .performFn([this](PerformContext*) { redoMove(); })
        .install();
}

wxWindow* MinesweeperBody::createFragmentView(CreateViewContext* ctx) {
    m_frame = ctx->findParentFrame();

    wxWindow* parent = ctx->getParent();
    
    m_statusText = new wxStaticText(parent, wxID_ANY, "Left click: reveal, Right click: flag");

    m_gridPanel = new wxPanel(parent, wxID_ANY);
    m_gridPanel->Bind(wxEVT_SIZE, &MinesweeperBody::onGridPanelSize, this);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(m_statusText, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(m_gridPanel, 1, wxEXPAND | wxALL, 5);
    parent->SetSizer(mainSizer);
    
    resetGame();

    return m_gridPanel;
}

wxEvtHandler* MinesweeperBody::getEventHandler() {
    return m_frame ? m_frame->GetEventHandler() : nullptr;
}

MinesweeperBody::GridSnap MinesweeperBody::captureSnap() const {
    GridSnap s;
    s.gameOver = m_gameOver;
    s.status = m_statusText ? m_statusText->GetLabel() : wxString();
    const int n = m_rows * m_cols;
    s.revealed.assign(n, false);
    s.flagged.assign(n, false);
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            const size_t i = static_cast<size_t>(r * m_cols + c);
            s.revealed[i] = m_grid[r][c].revealed;
            s.flagged[i] = m_grid[r][c].flagged;
        }
    }
    return s;
}

void MinesweeperBody::pushHistory() {
    m_undo.push_back(captureSnap());
    m_redo.clear();
}

void MinesweeperBody::applySnap(const GridSnap& s) {
    m_gameOver = s.gameOver;
    if (m_statusText)
        m_statusText->SetLabel(s.status);
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            const size_t i = static_cast<size_t>(r * m_cols + c);
            m_grid[r][c].revealed = s.revealed[i];
            m_grid[r][c].flagged = s.flagged[i];
        }
    }
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
            syncButton(r, c);
}

void MinesweeperBody::undoMove() {
    if (m_undo.empty())
        return;
    m_redo.push_back(captureSnap());
    applySnap(m_undo.back());
    m_undo.pop_back();
}

void MinesweeperBody::redoMove() {
    if (m_redo.empty())
        return;
    m_undo.push_back(captureSnap());
    applySnap(m_redo.back());
    m_redo.pop_back();
}

void MinesweeperBody::syncButton(int r, int c) {
    Cell& cell = m_grid[r][c];
    wxButton* btn = cell.button;
    if (!btn)
        return;
    btn->SetForegroundColour(wxNullColour);
    if (m_gameOver && cell.hasMine) {
        btn->SetLabel("X");
        btn->SetBackgroundColour(*wxRED);
        return;
    }
    if (!cell.revealed) {
        if (cell.flagged) {
            btn->SetLabel("F");
            btn->SetForegroundColour(*wxRED);
            btn->SetBackgroundColour(*wxLIGHT_GREY);
        } else {
            btn->SetLabel("");
            btn->SetBackgroundColour(*wxLIGHT_GREY);
        }
        return;
    }
    if (cell.hasMine) {
        btn->SetLabel("X");
        btn->SetBackgroundColour(*wxRED);
        return;
    }
    if (cell.adjacentMines > 0)
        btn->SetLabel(wxString::Format("%d", cell.adjacentMines));
    else
        btn->SetLabel("");
    btn->SetBackgroundColour(*wxWHITE);
}

void MinesweeperBody::resetGame() {
    m_gameOver = false;
    m_undo.clear();
    m_redo.clear();

    m_grid.assign(m_rows, std::vector<Cell>(m_cols));

    int placed = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dr(0, m_rows - 1);
    std::uniform_int_distribution<> dc(0, m_cols - 1);

    while (placed < m_mineCount) {
        int r = dr(gen);
        int c = dc(gen);
        if (!m_grid[r][c].hasMine) {
            m_grid[r][c].hasMine = true;
            placed++;
        }
    }

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            m_grid[r][c].adjacentMines = countAdjacentMines(r, c);
        }
    }

    layoutGrid();

    if (m_statusText) {
        m_statusText->SetLabel("Left click: reveal, Right click: flag");
    }
}

void MinesweeperBody::layoutGrid() {
    if (!m_gridPanel)
        return;

    m_gridPanel->DestroyChildren();

    wxGridSizer* sizer = new wxGridSizer(m_rows, m_cols, 0, 0);

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            wxButton* btn =
                new wxButton(m_gridPanel, wxID_ANY, "", wxDefaultPosition, wxSize(32, 32));
            m_grid[r][c].button = btn;
            btn->SetBackgroundColour(*wxLIGHT_GREY);

            btn->Bind(wxEVT_BUTTON, &MinesweeperBody::OnCellLeftClick, this);
            btn->Bind(wxEVT_RIGHT_DOWN, &MinesweeperBody::OnCellRightClick, this);

            sizer->Add(btn, 1, wxEXPAND | wxALL, 1);
        }
    }

    m_gridPanel->SetSizer(sizer);
    m_gridPanel->Layout();
    fitGridButtons();
    m_gridPanel->Refresh();
}

void MinesweeperBody::fitGridButtons() {
    if (!m_gridPanel || m_grid.empty() || m_grid[0].empty())
        return;
    wxSize cs = m_gridPanel->GetClientSize();
    if (cs.GetWidth() < 4 || cs.GetHeight() < 4)
        return;
    const int margin = 4;
    int cellW = (cs.GetWidth() - margin) / m_cols - 2;
    int cellH = (cs.GetHeight() - margin) / m_rows - 2;
    int cellSz = std::max(18, std::min(cellW, cellH));
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            wxButton* b = m_grid[r][c].button;
            if (b)
                b->SetMinSize(wxSize(cellSz, cellSz));
        }
    }
    m_gridPanel->Layout();
}

void MinesweeperBody::onGridPanelSize(wxSizeEvent& e) {
    fitGridButtons();
    e.Skip();
}

void MinesweeperBody::OnCellLeftClick(wxCommandEvent& event) {
    if (m_gameOver)
        return;

    wxObject* obj = event.GetEventObject();
    wxButton* btn = dynamic_cast<wxButton*>(obj);
    if (!btn)
        return;

    int r = -1, c = -1;
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            if (m_grid[i][j].button == btn) {
                r = i;
                c = j;
                break;
            }
        }
        if (r != -1)
            break;
    }

    if (r == -1 || c == -1)
        return;

    if (m_grid[r][c].flagged || m_grid[r][c].revealed)
        return;

    pushHistory();
    revealCell(r, c);
}

void MinesweeperBody::OnCellRightClick(wxMouseEvent& event) {
    if (m_gameOver)
        return;

    wxObject* obj = event.GetEventObject();
    wxButton* btn = dynamic_cast<wxButton*>(obj);
    if (!btn)
        return;

    int r = -1, c = -1;
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            if (m_grid[i][j].button == btn) {
                r = i;
                c = j;
                break;
            }
        }
        if (r != -1)
            break;
    }

    if (r == -1 || c == -1)
        return;

    Cell& cell = m_grid[r][c];
    if (cell.revealed)
        return;

    pushHistory();
    cell.flagged = !cell.flagged;
    if (cell.flagged) {
        btn->SetLabel("F");
        btn->SetForegroundColour(*wxRED);
    } else {
        btn->SetLabel("");
        btn->SetForegroundColour(wxNullColour);
    }
    event.Skip();
}

void MinesweeperBody::revealCell(int row, int col) {
    if (row < 0 || row >= m_rows || col < 0 || col >= m_cols)
        return;

    Cell& cell = m_grid[row][col];
    if (cell.revealed || cell.flagged)
        return;

    cell.revealed = true;

    if (cell.hasMine) {
        if (cell.button) {
            cell.button->SetLabel("X");
            cell.button->SetBackgroundColour(*wxRED);
        }
        gameOver(false);
        return;
    }

    if (cell.button) {
        if (cell.adjacentMines > 0) {
            cell.button->SetLabel(wxString::Format("%d", cell.adjacentMines));
        }
        cell.button->SetBackgroundColour(*wxWHITE);
    }

    if (cell.adjacentMines == 0) {
        revealEmptyNeighbors(row, col);
    }

    checkWinCondition();
}

void MinesweeperBody::revealEmptyNeighbors(int row, int col) {
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0)
                continue;
            int nr = row + dr;
            int nc = col + dc;
            if (nr < 0 || nr >= m_rows || nc < 0 || nc >= m_cols)
                continue;
            Cell& neighbor = m_grid[nr][nc];
            if (!neighbor.revealed && !neighbor.hasMine) {
                revealCell(nr, nc);
            }
        }
    }
}

int MinesweeperBody::countAdjacentMines(int row, int col) const {
    if (m_grid[row][col].hasMine)
        return 0;

    int count = 0;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0)
                continue;
            int nr = row + dr;
            int nc = col + dc;
            if (nr < 0 || nr >= m_rows || nc < 0 || nc >= m_cols)
                continue;
            if (m_grid[nr][nc].hasMine)
                ++count;
        }
    }
    return count;
}

void MinesweeperBody::checkWinCondition() {
    if (m_gameOver)
        return;

    int unrevealed = 0;
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            if (!m_grid[r][c].revealed)
                ++unrevealed;
        }
    }

    if (unrevealed == m_mineCount) {
        gameOver(true);
    }
}

void MinesweeperBody::gameOver(bool won) {
    m_gameOver = true;

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            Cell& cell = m_grid[r][c];
            if (cell.hasMine && cell.button) {
                cell.button->SetLabel("X");
                cell.button->SetBackgroundColour(*wxRED);
            }
        }
    }

    if (m_statusText) {
        m_statusText->SetLabel(won ? "You win! New game from toolbar."
                                   : "Boom! You lost. New game from toolbar.");
    }
}

} // namespace os

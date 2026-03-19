#include "MinesweeperApp.hpp"

#include "../../core/ModuleRegistry.hpp"

#include <wx/grid.h>

#include <random>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.minesweeper", MinesweeperApp)

MinesweeperApp::MinesweeperApp(CreateModuleContext* ctx)
    : Module(ctx) {
    initializeMetadata();
}

MinesweeperApp::~MinesweeperApp() {
    if (m_frame) {
        m_frame->Destroy();
        m_frame = nullptr;
    }
}

void MinesweeperApp::initializeMetadata() {
    uri = "omnishell";
    name = "minesweeper";
    label = "Minesweeper";
    description = "Classic minesweeper game";
    doc = "Simple playable minesweeper game.";
    categoryId = ID_CATEGORY_GAME;

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "bomb.svg"));
}

ProcessPtr MinesweeperApp::run() {
    if (m_frame) {
        m_frame->Raise();
        m_frame->SetFocus();
        auto p = std::make_shared<Process>();
        p->uri = uri;
        p->name = name;
        p->label = label;
        p->icon = image;
        p->addWindow(m_frame);
        return p;
    }

    createMainWindow();
    resetGame();
    m_frame->Show(true);
    auto p = std::make_shared<Process>();
    p->uri = uri;
    p->name = name;
    p->label = label;
    p->icon = image;
    p->addWindow(m_frame);
    return p;
}

void MinesweeperApp::createMainWindow() {
    m_frame = new wxFrame(nullptr, wxID_ANY, "Minesweeper", wxDefaultPosition, wxSize(500, 550));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    m_statusText = new wxStaticText(m_frame, wxID_ANY, "Left click: reveal, Right click: flag");
    mainSizer->Add(m_statusText, 0, wxEXPAND | wxALL, 5);

    m_gridPanel = new wxPanel(m_frame, wxID_ANY);
    mainSizer->Add(m_gridPanel, 1, wxEXPAND | wxALL, 5);

    wxButton* resetBtn = new wxButton(m_frame, wxID_ANY, "Reset");
    resetBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { resetGame(); });
    mainSizer->Add(resetBtn, 0, wxALIGN_RIGHT | wxALL, 5);

    m_frame->SetSizer(mainSizer);
    m_frame->Centre();
}

void MinesweeperApp::resetGame() {
    m_gameOver = false;

    m_grid.assign(m_rows, std::vector<Cell>(m_cols));

    // Place mines randomly
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

    // Precompute adjacent mine counts
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

void MinesweeperApp::layoutGrid() {
    if (!m_gridPanel)
        return;

    m_gridPanel->DestroyChildren();

    wxGridSizer* sizer = new wxGridSizer(m_rows, m_cols, 0, 0);

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            wxButton* btn = new wxButton(m_gridPanel, wxID_ANY, "",
                                         wxDefaultPosition, wxSize(32, 32));
            m_grid[r][c].button = btn;
            btn->SetBackgroundColour(*wxLIGHT_GREY);

            btn->Bind(wxEVT_BUTTON, &MinesweeperApp::OnCellLeftClick, this);
            btn->Bind(wxEVT_RIGHT_DOWN, &MinesweeperApp::OnCellRightClick, this);

            sizer->Add(btn, 1, wxEXPAND | wxALL, 1);
        }
    }

    m_gridPanel->SetSizer(sizer);
    m_gridPanel->Layout();
    m_gridPanel->Refresh();
}

void MinesweeperApp::OnCellLeftClick(wxCommandEvent& event) {
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

    revealCell(r, c);
}

void MinesweeperApp::OnCellRightClick(wxMouseEvent& event) {
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

    cell.flagged = !cell.flagged;
    if (cell.flagged) {
        btn->SetLabel("F");
        btn->SetForegroundColour(*wxRED);
    } else {
        btn->SetLabel("");
    }
    event.Skip();
}

void MinesweeperApp::revealCell(int row, int col) {
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

void MinesweeperApp::revealEmptyNeighbors(int row, int col) {
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

int MinesweeperApp::countAdjacentMines(int row, int col) const {
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

void MinesweeperApp::checkWinCondition() {
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

void MinesweeperApp::gameOver(bool won) {
    m_gameOver = true;

    // Reveal all mines
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
        m_statusText->SetLabel(won ? "You win! Click Reset to play again."
                                  : "Boom! You lost. Click Reset to try again.");
    }
}

} // namespace os


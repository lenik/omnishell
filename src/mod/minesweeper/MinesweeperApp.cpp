#include "MinesweeperApp.hpp"

#include "../../core/ModuleRegistry.hpp"

#include <random>
#include <wx/grid.h>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.minesweeper", MinesweeperApp)

MinesweeperApp::MinesweeperApp(CreateModuleContext* ctx)
    : Module(ctx) {
    initializeMetadata();
}

MinesweeperApp::~MinesweeperApp() {
    if (frame_) {
        frame_->Destroy();
        frame_ = nullptr;
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
    if (frame_) {
        frame_->Raise();
        frame_->SetFocus();
        auto p = std::make_shared<Process>();
        p->uri = uri;
        p->name = name;
        p->label = label;
        p->icon = image;
        p->addWindow(frame_);
        return p;
    }

    createMainWindow();
    resetGame();
    frame_->Show(true);
    auto p = std::make_shared<Process>();
    p->uri = uri;
    p->name = name;
    p->label = label;
    p->icon = image;
    p->addWindow(frame_);
    return p;
}

void MinesweeperApp::createMainWindow() {
    frame_ = new wxFrame(nullptr, wxID_ANY, "Minesweeper", wxDefaultPosition, wxSize(500, 550));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    statusText_ = new wxStaticText(frame_, wxID_ANY, "Left click: reveal, Right click: flag");
    mainSizer->Add(statusText_, 0, wxEXPAND | wxALL, 5);

    gridPanel_ = new wxPanel(frame_, wxID_ANY);
    mainSizer->Add(gridPanel_, 1, wxEXPAND | wxALL, 5);

    wxButton* resetBtn = new wxButton(frame_, wxID_ANY, "Reset");
    resetBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { resetGame(); });
    mainSizer->Add(resetBtn, 0, wxALIGN_RIGHT | wxALL, 5);

    frame_->SetSizer(mainSizer);
    frame_->Centre();
}

void MinesweeperApp::resetGame() {
    gameOver_ = false;

    grid_.assign(rows_, std::vector<Cell>(cols_));

    // Place mines randomly
    int placed = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dr(0, rows_ - 1);
    std::uniform_int_distribution<> dc(0, cols_ - 1);

    while (placed < mineCount_) {
        int r = dr(gen);
        int c = dc(gen);
        if (!grid_[r][c].hasMine) {
            grid_[r][c].hasMine = true;
            placed++;
        }
    }

    // Precompute adjacent mine counts
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            grid_[r][c].adjacentMines = countAdjacentMines(r, c);
        }
    }

    layoutGrid();

    if (statusText_) {
        statusText_->SetLabel("Left click: reveal, Right click: flag");
    }
}

void MinesweeperApp::layoutGrid() {
    if (!gridPanel_)
        return;

    gridPanel_->DestroyChildren();

    wxGridSizer* sizer = new wxGridSizer(rows_, cols_, 0, 0);

    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            wxButton* btn = new wxButton(gridPanel_, wxID_ANY, "",
                                         wxDefaultPosition, wxSize(32, 32));
            grid_[r][c].button = btn;
            btn->SetBackgroundColour(*wxLIGHT_GREY);

            btn->Bind(wxEVT_BUTTON, &MinesweeperApp::OnCellLeftClick, this);
            btn->Bind(wxEVT_RIGHT_DOWN, &MinesweeperApp::OnCellRightClick, this);

            sizer->Add(btn, 1, wxEXPAND | wxALL, 1);
        }
    }

    gridPanel_->SetSizer(sizer);
    gridPanel_->Layout();
    gridPanel_->Refresh();
}

void MinesweeperApp::OnCellLeftClick(wxCommandEvent& event) {
    if (gameOver_)
        return;

    wxObject* obj = event.GetEventObject();
    wxButton* btn = dynamic_cast<wxButton*>(obj);
    if (!btn)
        return;

    int r = -1, c = -1;
    for (int i = 0; i < rows_; ++i) {
        for (int j = 0; j < cols_; ++j) {
            if (grid_[i][j].button == btn) {
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

    if (grid_[r][c].flagged || grid_[r][c].revealed)
        return;

    revealCell(r, c);
}

void MinesweeperApp::OnCellRightClick(wxMouseEvent& event) {
    if (gameOver_)
        return;

    wxObject* obj = event.GetEventObject();
    wxButton* btn = dynamic_cast<wxButton*>(obj);
    if (!btn)
        return;

    int r = -1, c = -1;
    for (int i = 0; i < rows_; ++i) {
        for (int j = 0; j < cols_; ++j) {
            if (grid_[i][j].button == btn) {
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

    Cell& cell = grid_[r][c];
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
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_)
        return;

    Cell& cell = grid_[row][col];
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
            if (nr < 0 || nr >= rows_ || nc < 0 || nc >= cols_)
                continue;
            Cell& neighbor = grid_[nr][nc];
            if (!neighbor.revealed && !neighbor.hasMine) {
                revealCell(nr, nc);
            }
        }
    }
}

int MinesweeperApp::countAdjacentMines(int row, int col) const {
    if (grid_[row][col].hasMine)
        return 0;

    int count = 0;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0)
                continue;
            int nr = row + dr;
            int nc = col + dc;
            if (nr < 0 || nr >= rows_ || nc < 0 || nc >= cols_)
                continue;
            if (grid_[nr][nc].hasMine)
                ++count;
        }
    }
    return count;
}

void MinesweeperApp::checkWinCondition() {
    if (gameOver_)
        return;

    int unrevealed = 0;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (!grid_[r][c].revealed)
                ++unrevealed;
        }
    }

    if (unrevealed == mineCount_) {
        gameOver(true);
    }
}

void MinesweeperApp::gameOver(bool won) {
    gameOver_ = true;

    // Reveal all mines
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            Cell& cell = grid_[r][c];
            if (cell.hasMine && cell.button) {
                cell.button->SetLabel("X");
                cell.button->SetBackgroundColour(*wxRED);
            }
        }
    }

    if (statusText_) {
        statusText_->SetLabel(won ? "You win! Click Reset to play again."
                                  : "Boom! You lost. Click Reset to try again.");
    }
}

} // namespace os


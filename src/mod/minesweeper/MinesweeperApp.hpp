#ifndef OMNISHELL_APP_MINESWEEPER_HPP
#define OMNISHELL_APP_MINESWEEPER_HPP

#include "../../core/Module.hpp"

#include <wx/wx.h>
#include <vector>

namespace os {

class MinesweeperApp : public Module {
public:
    explicit MinesweeperApp(CreateModuleContext* ctx);
    virtual ~MinesweeperApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

private:
    struct Cell {
        bool hasMine{false};
        bool revealed{false};
        bool flagged{false};
        int adjacentMines{0};
        wxButton* button{nullptr};
    };

    void createMainWindow();
    void resetGame();
    void layoutGrid();

    void OnCellLeftClick(wxCommandEvent& event);
    void OnCellRightClick(wxMouseEvent& event);

    void revealCell(int row, int col);
    void revealEmptyNeighbors(int row, int col);
    int countAdjacentMines(int row, int col) const;
    void checkWinCondition();
    void gameOver(bool won);

    wxFrame* frame_{nullptr};
    wxPanel* gridPanel_{nullptr};
    wxStaticText* statusText_{nullptr};

    int rows_{9};
    int cols_{9};
    int mineCount_{10};
    bool gameOver_{false};

    std::vector<std::vector<Cell>> grid_;
};

} // namespace os

#endif // OMNISHELL_APP_MINESWEEPER_HPP


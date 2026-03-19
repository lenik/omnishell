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

    wxFrame* m_frame{nullptr};
    wxPanel* m_gridPanel{nullptr};
    wxStaticText* m_statusText{nullptr};

    int m_rows{9};
    int m_cols{9};
    int m_mineCount{10};
    bool m_gameOver{false};

    std::vector<std::vector<Cell>> m_grid;
};

} // namespace os

#endif // OMNISHELL_APP_MINESWEEPER_HPP


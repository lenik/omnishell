#ifndef OMNISHELL_MOD_MINESWEEPER_BODY_HPP
#define OMNISHELL_MOD_MINESWEEPER_BODY_HPP

#include "../../core/App.hpp"

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/wx.h>

#include <vector>

namespace os {

class MinesweeperBody : public UIFragment {
  public:
    explicit MinesweeperBody(App* app);
    ~MinesweeperBody() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

    void resetGame();

  private:
    struct Cell {
        bool hasMine{false};
        bool revealed{false};
        bool flagged{false};
        int adjacentMines{0};
        wxButton* button{nullptr};
    };

    struct GridSnap {
        bool gameOver{false};
        wxString status;
        std::vector<bool> revealed;
        std::vector<bool> flagged;
    };

    void layoutGrid();
    void fitGridButtons();
    void onGridPanelSize(wxSizeEvent& e);
    void syncButton(int row, int col);
    void pushHistory();
    void applySnap(const GridSnap& s);
    GridSnap captureSnap() const;

    void undoMove();
    void redoMove();

    void OnCellLeftClick(wxCommandEvent& event);
    void OnCellRightClick(wxMouseEvent& event);

    void revealCell(int row, int col);
    void revealEmptyNeighbors(int row, int col);
    int countAdjacentMines(int row, int col) const;
    void checkWinCondition();
    void gameOver(bool won);

    uiFrame* m_frame{nullptr};
    wxPanel* m_gridPanel{nullptr};
    wxStaticText* m_statusText{nullptr};

    int m_rows{9};
    int m_cols{9};
    int m_mineCount{10};
    bool m_gameOver{false};

    std::vector<std::vector<Cell>> m_grid;
    std::vector<GridSnap> m_undo;
    std::vector<GridSnap> m_redo;
};

} // namespace os

#endif

#ifndef OMNISHELL_MOD_SUDOKU_FRAME_HPP
#define OMNISHELL_MOD_SUDOKU_FRAME_HPP

#include "SudokuBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class SudokuFrame : public uiFrame {
  public:
    SudokuFrame(App* app, std::string title);
    ~SudokuFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

  private:
    SudokuBody m_body;
};

} // namespace os

#endif

#ifndef OMNISHELL_MOD_MINESWEEPER_FRAME_HPP
#define OMNISHELL_MOD_MINESWEEPER_FRAME_HPP

#include "MinesweeperBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class MinesweeperFrame : public uiFrame {
  public:
    MinesweeperFrame(App* app, std::string title);
    ~MinesweeperFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

    MinesweeperBody& body() { return m_body; }

  private:
    MinesweeperBody m_body;
};

} // namespace os

#endif

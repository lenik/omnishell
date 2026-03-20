#ifndef OMNISHELL_APP_MINESWEEPER_HPP
#define OMNISHELL_APP_MINESWEEPER_HPP

#include "../../core/Module.hpp"

namespace os {

class MinesweeperFrame;

class MinesweeperApp : public Module {
  public:
    explicit MinesweeperApp(CreateModuleContext* ctx);
    virtual ~MinesweeperApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

  private:
    MinesweeperFrame* m_frame{nullptr};
};

} // namespace os

#endif // OMNISHELL_APP_MINESWEEPER_HPP

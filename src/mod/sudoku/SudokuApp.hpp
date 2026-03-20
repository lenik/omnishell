#ifndef OMNISHELL_MOD_SUDOKU_APP_HPP
#define OMNISHELL_MOD_SUDOKU_APP_HPP

#include "../../core/Module.hpp"

namespace os {

class App;

class SudokuApp : public Module {
  public:
    explicit SudokuApp(CreateModuleContext* ctx);
    ~SudokuApp() override = default;

    ProcessPtr run() override;
    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif

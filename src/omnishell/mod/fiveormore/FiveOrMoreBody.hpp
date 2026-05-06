#ifndef OMNISHELL_MOD_FIVE_OR_MORE_BODY_HPP
#define OMNISHELL_MOD_FIVE_OR_MORE_BODY_HPP

#include "../../core/App.hpp"

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

namespace os {

class FiveOrMoreBody : public UIFragment {
  public:
    explicit FiveOrMoreBody(App* app);
    ~FiveOrMoreBody() override = default;

    void createFragmentView(CreateViewContext* ctx) override;

    /** Balls spawned after each completed move (1=easy .. 3=hard). */
    int chosenSpawnPerMove() const { return m_chosenLevel; }

    /** Canvas calls at start of a new game (enables difficulty UI). */
    void canvasNotifyNewGame();

    /** Canvas calls after each completed player move (locks difficulty until new game). */
    void canvasNotifyMoveFinished();

  private:
    void syncDifficultyToolbar(bool enable);

    wxWindow* m_canvas{nullptr};

    int m_chosenLevel{3};
    int m_movesCompleted{0};
};

} // namespace os

#endif

#ifndef OMNISHELL_MOD_SOLITAIRE_BODY_HPP
#define OMNISHELL_MOD_SOLITAIRE_BODY_HPP

#include "../../core/App.hpp"

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

class wxPanel;

namespace os {

class SolitaireBody : public UIFragment {
  public:
    explicit SolitaireBody(App* app);
    ~SolitaireBody() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

  private:
    uiFrame* m_frame{nullptr};
    wxPanel* m_canvas{nullptr};
};

} // namespace os

#endif

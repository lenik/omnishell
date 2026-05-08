#ifndef OMNISHELL_MOD_SOLITAIRE_FRAME_HPP
#define OMNISHELL_MOD_SOLITAIRE_FRAME_HPP

#include "SolitaireBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class SolitaireFrame : public uiFrame {
  public:
    SolitaireFrame(App* app, std::string title);
    ~SolitaireFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

  private:
    SolitaireBody m_body;
};

} // namespace os

#endif

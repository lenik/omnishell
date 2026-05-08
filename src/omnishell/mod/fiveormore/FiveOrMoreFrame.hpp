#ifndef OMNISHELL_MOD_FIVE_OR_MORE_FRAME_HPP
#define OMNISHELL_MOD_FIVE_OR_MORE_FRAME_HPP

#include "FiveOrMoreBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class FiveOrMoreFrame : public uiFrame {
  public:
    FiveOrMoreFrame(App* app, std::string title);
    ~FiveOrMoreFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

  private:
    FiveOrMoreBody m_body;
};

} // namespace os

#endif

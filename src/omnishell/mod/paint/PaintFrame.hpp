#ifndef OMNISHELL_MOD_PAINT_FRAME_HPP
#define OMNISHELL_MOD_PAINT_FRAME_HPP

#include "PaintBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class PaintFrame : public uiFrame {
  public:
    PaintFrame(App* app, std::string title);
    ~PaintFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

    PaintBody& body() { return m_body; }

  private:
    PaintBody m_body;
};

} // namespace os

#endif

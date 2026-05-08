#ifndef OMNISHELL_MOD_CALENDAR_FRAME_HPP
#define OMNISHELL_MOD_CALENDAR_FRAME_HPP

#include "CalendarBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class CalendarFrame : public uiFrame {
  public:
    CalendarFrame(App* app, std::string title);
    ~CalendarFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

  private:
    CalendarBody m_body;
};

} // namespace os

#endif

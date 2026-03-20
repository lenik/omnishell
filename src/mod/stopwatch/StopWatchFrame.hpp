#ifndef OMNISHELL_MOD_STOPWATCH_FRAME_HPP
#define OMNISHELL_MOD_STOPWATCH_FRAME_HPP

#include "StopWatchBody.hpp"

#include "../../core/App.hpp"

#include <bas/wx/uiframe.hpp>

#include <string>

namespace os {

class StopWatchFrame : public uiFrame {
  public:
    StopWatchFrame(App* app, std::string title);
    ~StopWatchFrame() override = default;

    void createFragmentView(CreateViewContext* ctx) override;

  private:
    StopWatchBody m_body;
};

} // namespace os

#endif

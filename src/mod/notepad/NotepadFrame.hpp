#ifndef NOTEPAD_FRAME_HPP
#define NOTEPAD_FRAME_HPP

#include "NotepadCore.hpp"

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

namespace os {

class NotepadFrame : public uiFrame {
  public:
    NotepadFrame();
    ~NotepadFrame() override = default;

    void createFragmentView(CreateViewContext* ctx) override;

  private:
    NotepadCore m_core;
};

} // namespace os

#endif // NOTEPAD_FRAME_HPP

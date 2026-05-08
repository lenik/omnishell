#ifndef NOTEPAD_FRAME_HPP
#define NOTEPAD_FRAME_HPP

#include "NotepadBody.hpp"

#include "../../core/App.hpp"

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

namespace os {

class NotepadFrame : public uiFrame {
  public:
    NotepadFrame(App* app, std::string title);
    ~NotepadFrame() override = default;

    wxWindow* createFragmentView(CreateViewContext* ctx) override;

    NotepadBody& body() { return m_body; }

  private:
    NotepadBody m_body;
};

} // namespace os

#endif // NOTEPAD_FRAME_HPP

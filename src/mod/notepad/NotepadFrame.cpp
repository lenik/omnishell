#include "NotepadFrame.hpp"

namespace os {

NotepadFrame::NotepadFrame() //
    : uiFrame("Notepad") {
    addFragment(&m_core);
    createView();
}

void NotepadFrame::createFragmentView(CreateViewContext* ctx) {}

} // namespace os

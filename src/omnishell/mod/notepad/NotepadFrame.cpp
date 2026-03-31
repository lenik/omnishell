#include "NotepadFrame.hpp"

namespace os {

NotepadFrame::NotepadFrame(App* app, std::string title) //
    : uiFrame(title), m_body(app)                       //
{
    addFragment(&m_body);
    createView();

    // createMenuBar();
    // editor_ = new wxTextCtrl(frame_, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
    //                          wxTE_MULTILINE | wxTE_RICH2 | wxTE_PROCESS_ENTER);
    // wxFont font(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    // editor_->SetFont(font);
    // statusBar_ = frame_->CreateStatusBar(2);
    // statusBar_->SetStatusText("Line 1, Column 1", 0);
    // statusBar_->SetStatusText("UTF-8", 1);
}

void NotepadFrame::createFragmentView(CreateViewContext* ctx) {
    uiFrame::createFragmentView(ctx);
}

} // namespace os

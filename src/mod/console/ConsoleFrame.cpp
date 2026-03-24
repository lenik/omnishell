#include "ConsoleFrame.hpp"

#include "../../core/App.hpp"

#include <wx/sizer.h>

namespace os {

ConsoleFrame::ConsoleFrame(App* app)
    : wxFrame(nullptr, wxID_ANY, wxT("Console"), wxDefaultPosition, wxSize(800, 400))
    , m_body(new ConsoleBody(this)) {
    (void)app;
    SetName(wxT("console"));
    m_body->SetName(wxT("console_body"));

    wxBoxSizer* sz = new wxBoxSizer(wxVERTICAL);
    sz->Add(m_body, 1, wxEXPAND | wxALL, 6);
    SetSizer(sz);
}

} // namespace os

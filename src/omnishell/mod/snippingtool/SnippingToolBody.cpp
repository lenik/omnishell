#include "SnippingToolBody.hpp"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

namespace os {

namespace {
enum {
    ID_NEW_SNIP = wxID_HIGHEST + 1,
    ID_SAVE,
    ID_COPY,
    ID_MODE
};
}

void SnippingToolBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    m_panel = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    m_panel->SetMinSize(wxSize(400, 300));

    auto* mainsizer = new wxBoxSizer(wxVERTICAL);

    auto* toolsizer = new wxBoxSizer(wxHORIZONTAL);
    toolsizer->Add(new wxButton(m_panel, ID_NEW_SNIP, "New Snip"), 0, wxALL, 5);
    toolsizer->Add(new wxButton(m_panel, ID_SAVE, "Save"), 0, wxALL, 5);
    toolsizer->Add(new wxButton(m_panel, ID_COPY, "Copy"), 0, wxALL, 5);
    toolsizer->Add(new wxStaticText(m_panel, wxID_ANY, "Mode:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    m_modeChoice = new wxChoice(m_panel, ID_MODE);
    m_modeChoice->Append("Free-form");
    m_modeChoice->Append("Rectangular");
    m_modeChoice->Append("Window");
    m_modeChoice->Append("Full-screen");
    m_modeChoice->SetSelection(1);
    toolsizer->Add(m_modeChoice, 0, wxALL, 5);
    mainsizer->Add(toolsizer, 0, wxEXPAND);

    m_preview = new wxStaticBitmap(m_panel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                   wxSUNKEN_BORDER);
    mainsizer->Add(m_preview, 1, wxEXPAND | wxALL, 10);

    mainsizer->Add(new wxStaticText(m_panel, wxID_ANY,
                                    "Click 'New Snip' to capture a screenshot. Select the capture mode above."),
                   0, wxALL | wxALIGN_CENTER, 10);

    m_panel->SetSizer(mainsizer);

    m_panel->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        if (e.GetId() == ID_NEW_SNIP)
            OnNewSnip(e);
        else if (e.GetId() == ID_SAVE)
            OnSave(e);
        else if (e.GetId() == ID_COPY)
            OnCopy(e);
    });
    m_panel->Bind(wxEVT_CHOICE, &SnippingToolBody::OnMode, this, ID_MODE);
}

void SnippingToolBody::OnNewSnip(wxCommandEvent& event) {
    (void)event;

    m_capturedBitmap = wxBitmap(400, 300, 24);
    wxMemoryDC dc(m_capturedBitmap);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    dc.SetPen(*wxBLACK_PEN);
    dc.DrawRectangle(0, 0, 400, 300);
    dc.DrawText("Screenshot captured", 130, 140);

    if (m_preview)
        m_preview->SetBitmap(m_capturedBitmap);
}

void SnippingToolBody::OnSave(wxCommandEvent& event) {
    (void)event;

    if (!m_capturedBitmap.IsOk()) {
        wxMessageBox("No screenshot captured yet.", "Snipping Tool", wxOK | wxICON_INFORMATION);
        return;
    }

    wxWindow* dlgParent = m_frame ? static_cast<wxWindow*>(m_frame) : m_panel;
    wxFileDialog dlg(dlgParent, "Save Screenshot", "", "screenshot.png",
                     "PNG files (*.png)|*.png|JPEG files (*.jpg)|*.jpg", wxFD_SAVE);
    if (dlg.ShowModal() == wxID_OK) {
        m_capturedBitmap.SaveFile(dlg.GetPath(), wxBITMAP_TYPE_PNG);
        wxMessageBox("Screenshot saved.", "Snipping Tool", wxOK | wxICON_INFORMATION);
    }
}

void SnippingToolBody::OnCopy(wxCommandEvent& event) {
    (void)event;

    if (!m_capturedBitmap.IsOk()) {
        wxMessageBox("No screenshot captured yet.", "Snipping Tool", wxOK | wxICON_INFORMATION);
        return;
    }

    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxBitmapDataObject(m_capturedBitmap));
        wxTheClipboard->Close();
        wxMessageBox("Screenshot copied to clipboard.", "Snipping Tool", wxOK | wxICON_INFORMATION);
    }
}

void SnippingToolBody::OnMode(wxCommandEvent& event) {
    (void)event;
}

} // namespace os

#include "LabeledIcon.hpp"

#include <wx/dcbuffer.h>
#include <wx/utils.h>

namespace {

// Wrap text to a maximum pixel width, breaking at characters when needed.
wxString wrapTextToWidth(const wxString& text, int maxWidth, wxWindow* window,
                         wxStaticText* labelCtrl) {
    if (maxWidth <= 0 || !window || !labelCtrl) {
        return text;
    }

    wxClientDC dc(window);
    dc.SetFont(labelCtrl->GetFont());

    wxString result;
    int lineWidth = 0;

    for (wxUniChar ch : text) {
        if (ch == '\n') {
            result += ch;
            lineWidth = 0;
            continue;
        }

        wxString s(ch);
        int cw = 0, chH = 0;
        dc.GetTextExtent(s, &cw, &chH);

        if (lineWidth > 0 && lineWidth + cw > maxWidth) {
            // Break line before this character
            result += '\n';
            lineWidth = 0;
        }

        result += ch;
        lineWidth += cw;
    }

    return result;
}

} // namespace

namespace os {

LabeledIcon::LabeledIcon(wxWindow* parent, wxWindowID id, const wxBitmap& bitmap,
                         const wxString& label, const wxPoint& pos, const wxSize& size,
                         long style)
    : wxPanel(parent, id, pos, wxDefaultSize, style), m_bitmap(nullptr), m_label(nullptr) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_bitmap = new wxStaticBitmap(this, wxID_ANY, bitmap);
    m_label = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize,
                              wxALIGN_CENTRE);

    // Fixed wrapping width; allow height to grow naturally.
    const int wrapWidth = 80;
    m_label->SetLabel(label);
    m_label->Wrap(wrapWidth);

    // Padding inside icon: 8px vertical, 12px horizontal, 5px between image & label
    sizer->AddSpacer(8); // top padding
    sizer->Add(m_bitmap, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(5); // gap between image and label
    sizer->Add(m_label, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(8); // bottom padding

    SetSizerAndFit(sizer);

    // Forward child mouse events so external bindings on LabeledIcon work
    auto forwardLeftDClick = [this](wxMouseEvent& e) {
        wxMouseEvent evt(e);
        evt.SetEventObject(this);
        ProcessWindowEvent(evt);
        e.Skip();
    };
    auto forwardRightDown = [this](wxMouseEvent& e) {
        wxMouseEvent evt(e);
        evt.SetEventObject(this);
        ProcessWindowEvent(evt);
        e.Skip();
    };

    m_bitmap->Bind(wxEVT_LEFT_DCLICK, forwardLeftDClick);
    m_label->Bind(wxEVT_LEFT_DCLICK, forwardLeftDClick);
    m_bitmap->Bind(wxEVT_RIGHT_DOWN, forwardRightDown);
    m_label->Bind(wxEVT_RIGHT_DOWN, forwardRightDown);

    // Forward drag-related mouse events from children as well
    auto forwardLeftDown = [this](wxMouseEvent& e) {
        wxMouseEvent evt(e);
        evt.SetEventObject(this);
        ProcessWindowEvent(evt);
        e.Skip();
    };
    auto forwardLeftUp = [this](wxMouseEvent& e) {
        wxMouseEvent evt(e);
        evt.SetEventObject(this);
        ProcessWindowEvent(evt);
        e.Skip();
    };
    auto forwardMotion = [this](wxMouseEvent& e) {
        wxMouseEvent evt(e);
        evt.SetEventObject(this);
        ProcessWindowEvent(evt);
        e.Skip();
    };

    m_bitmap->Bind(wxEVT_LEFT_DOWN, forwardLeftDown);
    m_label->Bind(wxEVT_LEFT_DOWN, forwardLeftDown);
    m_bitmap->Bind(wxEVT_LEFT_UP, forwardLeftUp);
    m_label->Bind(wxEVT_LEFT_UP, forwardLeftUp);
    m_bitmap->Bind(wxEVT_MOTION, forwardMotion);
    m_label->Bind(wxEVT_MOTION, forwardMotion);

    // Hover / pressed tracking on panel
    Bind(wxEVT_ENTER_WINDOW, &LabeledIcon::OnMouseEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, &LabeledIcon::OnMouseLeave, this);
    Bind(wxEVT_LEFT_DOWN, &LabeledIcon::OnMouseDown, this);
    Bind(wxEVT_LEFT_UP, &LabeledIcon::OnMouseUp, this);
    Bind(wxEVT_PAINT, &LabeledIcon::OnPaint, this);

    // Also update hover state when moving over children
    auto childEnter = [this](wxMouseEvent& e) {
        m_hover = true;
        Refresh();
        e.Skip();
    };
    auto childLeave = [this](wxMouseEvent& e) {
        wxPoint screenPos = wxGetMousePosition();
        wxPoint clientPos = ScreenToClient(screenPos);
        m_hover = GetClientRect().Contains(clientPos);
        if (!m_hover) {
            m_pressed = false;
        }
        Refresh();
        e.Skip();
    };

    m_bitmap->Bind(wxEVT_ENTER_WINDOW, childEnter);
    m_label->Bind(wxEVT_ENTER_WINDOW, childEnter);
    m_bitmap->Bind(wxEVT_LEAVE_WINDOW, childLeave);
    m_label->Bind(wxEVT_LEAVE_WINDOW, childLeave);
}

void LabeledIcon::setBitmap(const wxBitmap& bitmap) {
    if (m_bitmap) {
        m_bitmap->SetBitmap(bitmap);
        Layout();
        Refresh();
    }
}

void LabeledIcon::setLabel(const wxString& text) {
    if (m_label) {
        m_label->SetLabel(text);
        m_label->Wrap(80);
        Layout();
        Refresh();
    }
}

void LabeledIcon::OnPaint(wxPaintEvent& event) {
    // Transparent background by default; only draw our highlight when needed.
    if (!m_hover && !m_pressed) {
        event.Skip();
        return;
    }

    wxAutoBufferedPaintDC dc(this);
    wxRect rect = GetClientRect();

    wxRect box = rect;
    box.Deflate(2);

    wxColour borderColor(200, 200, 200); // light gray border
    wxColour fillColor;
    if (m_pressed) {
        fillColor = wxColour(200, 220, 245); // pressed background
    } else {
        fillColor = wxColour(220, 235, 250); // hover background
    }

    dc.SetPen(wxPen(borderColor, 1));
    dc.SetBrush(wxBrush(fillColor));
    dc.DrawRoundedRectangle(box, 4);

    // Let children paint on top
    event.Skip();
}

void LabeledIcon::OnMouseEnter(wxMouseEvent& event) {
    m_hover = true;
    Refresh();
    event.Skip();
}

void LabeledIcon::OnMouseLeave(wxMouseEvent& event) {
    wxPoint screenPos = wxGetMousePosition();
    wxPoint clientPos = ScreenToClient(screenPos);
    m_hover = GetClientRect().Contains(clientPos);
    if (!m_hover) {
        m_pressed = false;
    }
    Refresh();
    event.Skip();
}

void LabeledIcon::OnMouseDown(wxMouseEvent& event) {
    m_pressed = true;
    if (!HasCapture()) {
        CaptureMouse();
    }
    Refresh();
    event.Skip();
}

void LabeledIcon::OnMouseUp(wxMouseEvent& event) {
    m_pressed = false;
    if (HasCapture()) {
        ReleaseMouse();
    }
    Refresh();
    event.Skip();
}

} // namespace os
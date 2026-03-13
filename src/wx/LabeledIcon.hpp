#ifndef OMNISHELL_UI_WX_LABELED_ICON_HPP
#define OMNISHELL_UI_WX_LABELED_ICON_HPP

#include <wx/panel.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/event.h>

namespace os {

/**
 * LabeledIcon
 *
 * Composite widget: icon bitmap + text label stacked vertically.
 * Exposes simple events for double-click and right-click.
 */
class LabeledIcon : public wxPanel {
public:
    LabeledIcon(wxWindow* parent, wxWindowID id = wxID_ANY,
                const wxBitmap& bitmap = wxNullBitmap,
                const wxString& label = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxTAB_TRAVERSAL);

    void setBitmap(const wxBitmap& bitmap);
    void setLabel(const wxString& label);

    wxStaticBitmap* getBitmapCtrl() const { return bitmap_; }
    wxStaticText* getLabelCtrl() const { return label_; }

    // Convenience: forward these to child controls
    template <typename Func>
    void onLeftDClick(Func&& handler) {
        Bind(wxEVT_LEFT_DCLICK, std::forward<Func>(handler));
    }

    template <typename Func>
    void onRightDown(Func&& handler) {
        Bind(wxEVT_RIGHT_DOWN, std::forward<Func>(handler));
    }

private:
    void OnPaint(wxPaintEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);

private:
    wxStaticBitmap* bitmap_;
    wxStaticText* label_;
    bool hover_ = false;
    bool pressed_ = false;
};

} // namespace os

#endif // OMNISHELL_UI_WX_LABELED_ICON_HPP


#ifndef OMNISHELL_MOD_PAINT_CORE_HPP
#define OMNISHELL_MOD_PAINT_CORE_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/image.h>
#include <wx/panel.h>

#include <cstdint>
#include <vector>

class wxBitmapToggleButton;

namespace os {

class PaintBody : public UIFragment {
public:
    enum class PaintTool : std::uint8_t {
        Pencil,
        Brush,
        Eraser,
        Line,
        Rect,
        Ellipse,
        Fill,
        Picker,
        Text
    };

    PaintBody();
    ~PaintBody() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

    /** Load an image into the canvas (used by Explorer). */
    void loadImage(const wxImage& img);

private:
    class PaintCanvas;

    uiFrame* m_frame{nullptr};
    wxPanel* m_root{nullptr};
    PaintCanvas* m_canvas{nullptr};
    wxImage m_pendingImage;

    std::vector<std::pair<wxBitmapToggleButton*, PaintTool>> m_toolToggles;

    void setCurrentTool(PaintTool t);

    void onSavePng(PerformContext* ctx);
    void onClear(PerformContext* ctx);

    void onToolPencil(PerformContext* ctx);
    void onToolBrush(PerformContext* ctx);
    void onToolEraser(PerformContext* ctx);
    void onToolLine(PerformContext* ctx);
    void onToolRect(PerformContext* ctx);
    void onToolEllipse(PerformContext* ctx);
    void onToolFill(PerformContext* ctx);
    void onToolPicker(PerformContext* ctx);
    void onToolText(PerformContext* ctx);
};

} // namespace os

#endif

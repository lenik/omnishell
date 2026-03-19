#ifndef OMNISHELL_MOD_PAINT_CORE_HPP
#define OMNISHELL_MOD_PAINT_CORE_HPP

#include <bas/ui/arch/UIFragment.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/panel.h>
#include <wx/image.h>

namespace os {

class PaintCore : public UIFragment {
public:
    PaintCore();
    ~PaintCore() override = default;

    void createFragmentView(CreateViewContext* ctx) override;
    wxEvtHandler* getEventHandler() override;

    /** Load an image into the canvas (used by Explorer). */
    void loadImage(const wxImage& img);

private:
    class PaintCanvas;

    uiFrame* frame_{nullptr};
    wxPanel* root_{nullptr};
    PaintCanvas* canvas_{nullptr};
    wxImage pendingImage_;

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


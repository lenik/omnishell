#include "PaintBody.hpp"

#include <bas/ui/arch/ImageSet.hpp>

#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/dcbuffer.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textdlg.h>
#include <wx/tglbtn.h>

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace os {

namespace {

using PaintTool = PaintBody::PaintTool;

enum {
    ID_SAVE_PNG = uiFrame::ID_APP_HIGHEST + 1,
    ID_CLEAR,
    ID_TOOL_PENCIL,
    ID_TOOL_BRUSH,
    ID_TOOL_ERASER,
    ID_TOOL_LINE,
    ID_TOOL_RECT,
    ID_TOOL_ELLIPSE,
    ID_TOOL_FILL,
    ID_TOOL_PICKER,
    ID_TOOL_TEXT,
    ID_GROUP_TOOLS = uiFrame::ID_APP_HIGHEST + 100,
    ID_GROUP_TOOLS_DRAW,
    ID_GROUP_TOOLS_SHAPES,
};

} // namespace

class PaintBody::PaintCanvas : public wxPanel {
public:
    PaintCanvas(wxWindow* parent)
        : wxPanel(parent, wxID_ANY) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &PaintCanvas::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &PaintCanvas::OnLeftDown, this);
        Bind(wxEVT_LEFT_UP, &PaintCanvas::OnLeftUp, this);
        Bind(wxEVT_MOTION, &PaintCanvas::OnMove, this);
        Bind(wxEVT_SIZE, &PaintCanvas::OnSize, this);
    }

    void setPrimaryColor(const wxColour& c) { m_primary = c; }
    void setWidth(int w) { m_width = std::max(1, w); }
    void setTool(PaintTool t) { m_tool = t; }

    void clear() {
        ensureBuffer();
        wxMemoryDC dc(m_buffer);
        dc.SetBackground(wxBrush(*wxWHITE));
        dc.Clear();
        m_previewActive = false;
        Refresh();
    }

    bool savePng(const wxString& path) {
        ensureBuffer();
        wxImage img = m_buffer.ConvertToImage();
        return img.IsOk() && img.SaveFile(path, wxBITMAP_TYPE_PNG);
    }

    void loadImage(const wxImage& img) {
        if (!img.IsOk())
            return;
        wxSize sz = GetClientSize();
        wxImage scaled = img;
        if (sz.GetWidth() > 0 && sz.GetHeight() > 0) {
            scaled = img.Scale(sz.GetWidth(), sz.GetHeight(), wxIMAGE_QUALITY_HIGH);
        }
        m_buffer = wxBitmap(scaled);
        m_previewActive = false;
        Refresh();
    }

private:
    void ensureBuffer() {
        wxSize sz = GetClientSize();
        if (sz.GetWidth() <= 0 || sz.GetHeight() <= 0)
            return;
        if (!m_buffer.IsOk() || m_buffer.GetWidth() != sz.GetWidth() || m_buffer.GetHeight() != sz.GetHeight()) {
            wxBitmap newBuf(sz.GetWidth(), sz.GetHeight());
            wxMemoryDC dc(newBuf);
            dc.SetBackground(wxBrush(*wxWHITE));
            dc.Clear();
            if (m_buffer.IsOk()) {
                wxMemoryDC old(m_buffer);
                dc.Blit(0, 0, m_buffer.GetWidth(), m_buffer.GetHeight(), &old, 0, 0);
            }
            m_buffer = newBuf;
        }
    }

    wxPen makePen(bool erasing) const {
        wxPen pen(erasing ? *wxWHITE : m_primary, m_width, wxPENSTYLE_SOLID);
        pen.SetCap(wxCAP_ROUND);
        pen.SetJoin(wxJOIN_ROUND);
        return pen;
    }

    void drawStroke(const wxPoint& a, const wxPoint& b, bool erasing) {
        ensureBuffer();
        wxMemoryDC dc(m_buffer);
        dc.SetPen(makePen(erasing));
        dc.DrawLine(a, b);
    }

    void drawShapeTo(wxDC& dc, PaintTool shape, const wxPoint& a, const wxPoint& b) {
        wxPen pen = makePen(false);
        dc.SetPen(pen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        if (shape == PaintTool::Line) {
            dc.DrawLine(a, b);
            return;
        }
        int x = std::min(a.x, b.x);
        int y = std::min(a.y, b.y);
        int w = std::abs(a.x - b.x);
        int h = std::abs(a.y - b.y);
        if (shape == PaintTool::Rect) {
            dc.DrawRectangle(x, y, w, h);
        } else if (shape == PaintTool::Ellipse) {
            dc.DrawEllipse(x, y, w, h);
        }
    }

    static bool colorEquals(const wxImage& img, int x, int y, const wxColour& c) {
        return img.GetRed(x, y) == c.Red() && img.GetGreen(x, y) == c.Green() && img.GetBlue(x, y) == c.Blue();
    }

    void floodFill(const wxPoint& pt, const wxColour& fillColor) {
        ensureBuffer();
        if (!m_buffer.IsOk())
            return;
        wxImage img = m_buffer.ConvertToImage();
        if (!img.IsOk())
            return;
        const int w = img.GetWidth();
        const int h = img.GetHeight();
        if (pt.x < 0 || pt.y < 0 || pt.x >= w || pt.y >= h)
            return;

        wxColour target(img.GetRed(pt.x, pt.y), img.GetGreen(pt.x, pt.y), img.GetBlue(pt.x, pt.y));
        if (target == fillColor)
            return;

        std::vector<uint8_t> visited((size_t)w * (size_t)h, 0);
        std::vector<wxPoint> stack;
        stack.reserve(4096);
        stack.push_back(pt);
        while (!stack.empty()) {
            wxPoint p = stack.back();
            stack.pop_back();
            if (p.x < 0 || p.y < 0 || p.x >= w || p.y >= h)
                continue;
            size_t idx = (size_t)p.y * (size_t)w + (size_t)p.x;
            if (visited[idx])
                continue;
            visited[idx] = 1;
            if (!colorEquals(img, p.x, p.y, target))
                continue;
            img.SetRGB(p.x, p.y, fillColor.Red(), fillColor.Green(), fillColor.Blue());
            stack.push_back(wxPoint(p.x + 1, p.y));
            stack.push_back(wxPoint(p.x - 1, p.y));
            stack.push_back(wxPoint(p.x, p.y + 1));
            stack.push_back(wxPoint(p.x, p.y - 1));
        }
        m_buffer = wxBitmap(img);
        m_previewActive = false;
        Refresh(false);
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(*wxWHITE));
        dc.Clear();
        if (m_buffer.IsOk()) {
            dc.DrawBitmap(m_buffer, 0, 0, false);
        }
        if (m_previewActive && (m_tool == PaintTool::Line || m_tool == PaintTool::Rect || m_tool == PaintTool::Ellipse)) {
            drawShapeTo(dc, m_tool, m_dragStart, m_last);
        }
    }

    void OnLeftDown(wxMouseEvent& e) {
        ensureBuffer();
        CaptureMouse();
        m_dragging = true;
        m_dragStart = e.GetPosition();
        m_last = m_dragStart;

        if (m_tool == PaintTool::Picker) {
            if (m_buffer.IsOk()) {
                wxImage img = m_buffer.ConvertToImage();
                if (img.IsOk() && m_dragStart.x >= 0 && m_dragStart.y >= 0 &&
                    m_dragStart.x < img.GetWidth() && m_dragStart.y < img.GetHeight()) {
                    m_primary = wxColour(img.GetRed(m_dragStart.x, m_dragStart.y),
                                        img.GetGreen(m_dragStart.x, m_dragStart.y),
                                        img.GetBlue(m_dragStart.x, m_dragStart.y));
                }
            }
            m_dragging = false;
            if (HasCapture())
                ReleaseMouse();
            return;
        }

        if (m_tool == PaintTool::Fill) {
            floodFill(m_dragStart, m_primary);
            m_dragging = false;
            if (HasCapture())
                ReleaseMouse();
            return;
        }

        if (m_tool == PaintTool::Text) {
            wxTextEntryDialog dlg(this, "Text:", "Paint");
            if (dlg.ShowModal() == wxID_OK) {
                wxString text = dlg.GetValue();
                if (!text.IsEmpty()) {
                    wxMemoryDC dc(m_buffer);
                    dc.SetTextForeground(m_primary);
                    dc.DrawText(text, m_dragStart);
                    Refresh(false);
                }
            }
            m_dragging = false;
            if (HasCapture())
                ReleaseMouse();
            return;
        }

        m_previewActive = (m_tool == PaintTool::Line || m_tool == PaintTool::Rect || m_tool == PaintTool::Ellipse);
    }

    void OnLeftUp(wxMouseEvent&) {
        if (m_previewActive && (m_tool == PaintTool::Line || m_tool == PaintTool::Rect || m_tool == PaintTool::Ellipse)) {
            ensureBuffer();
            wxMemoryDC dc(m_buffer);
            drawShapeTo(dc, m_tool, m_dragStart, m_last);
            m_previewActive = false;
            Refresh(false);
        }
        m_dragging = false;
        if (HasCapture())
            ReleaseMouse();
    }

    void OnMove(wxMouseEvent& e) {
        if (!m_dragging || !e.LeftIsDown())
            return;
        wxPoint p = e.GetPosition();
        if (m_tool == PaintTool::Pencil) {
            drawStroke(m_last, p, false);
        } else if (m_tool == PaintTool::Brush) {
            int oldW = m_width;
            m_width = std::max(m_width, 6);
            drawStroke(m_last, p, false);
            m_width = oldW;
        } else if (m_tool == PaintTool::Eraser) {
            drawStroke(m_last, p, true);
        } else if (m_tool == PaintTool::Line || m_tool == PaintTool::Rect || m_tool == PaintTool::Ellipse) {
            m_previewActive = true;
        }
        m_last = p;
        Refresh(false);
    }

    void OnSize(wxSizeEvent& e) {
        ensureBuffer();
        e.Skip();
        Refresh();
    }

    wxBitmap m_buffer;
    wxColour m_primary{0, 0, 0};
    int m_width{3};
    PaintTool m_tool{PaintTool::Pencil};
    bool m_dragging{false};
    wxPoint m_last{0, 0};
    wxPoint m_dragStart{0, 0};
    bool m_previewActive{false};
};

PaintBody::PaintBody() {
    std::string dir = "heroicons/normal";

    // Top-level menu groups (force consistent menu ordering + explicit submenus).
    group(ID_GROUP_TOOLS, "tools", "_root", 1000, "&Tools", "Tools menu").install();
    group(ID_GROUP_TOOLS_DRAW, "tools", "draw", 1010, "&Draw", "Drawing tools").install();
    group(ID_GROUP_TOOLS_SHAPES, "tools", "shapes", 1020, "&Shapes", "Shape tools").install();

    int seq = 0;
    action(ID_SAVE_PNG, "file", "save_png", seq++, "Save &PNG...", "Save image as PNG")
        .icon(wxART_FILE_SAVE_AS, dir, "arrow-down-tray.svg")
        .performFn([this](PerformContext* ctx) { onSavePng(ctx); })
        .install();
    action(ID_CLEAR, "edit", "clear", seq++, "&Clear", "Clear canvas")
        .icon(wxART_DELETE, dir, "trash.svg")
        .performFn([this](PerformContext* ctx) { onClear(ctx); })
        .install();

    seq = 0;
    action(ID_TOOL_PENCIL, "tools/draw", "pencil", seq++, "&Pencil", "Pencil tool")
        .icon(wxART_TIP, dir, "pencil.svg")
        .no_tool()
        .performFn([this](PerformContext* ctx) { onToolPencil(ctx); })
        .install();
    action(ID_TOOL_BRUSH, "tools/draw", "brush", seq++, "&Brush", "Brush tool")
        .icon(wxART_TIP, dir, "paint-brush.svg")
        .no_tool()
        .performFn([this](PerformContext* ctx) { onToolBrush(ctx); })
        .install();
    action(ID_TOOL_ERASER, "tools/draw", "eraser", seq++, "&Eraser", "Eraser tool")
        .icon(wxART_DELETE, dir, "backspace.svg")
        .no_tool()
        .performFn([this](PerformContext* ctx) { onToolEraser(ctx); })
        .install();

    seq = 0;
    action(ID_TOOL_LINE, "tools/shapes", "line", seq++, "&Line", "Line tool")
        .icon(wxART_MINUS, dir, "minus.svg")
        .no_tool()
        .performFn([this](PerformContext* ctx) { onToolLine(ctx); })
        .install();
    action(ID_TOOL_RECT, "tools/shapes", "rect", seq++, "&Rectangle", "Rectangle tool")
        .icon(wxART_NORMAL_FILE, dir, "rectangle-group.svg")
        .no_tool()
        .performFn([this](PerformContext* ctx) { onToolRect(ctx); })
        .install();
    action(ID_TOOL_ELLIPSE, "tools/shapes", "ellipse", seq++, "&Ellipse", "Ellipse tool")
        .icon(wxART_NORMAL_FILE, dir, "stop-circle.svg")
        .no_tool()
        .performFn([this](PerformContext* ctx) { onToolEllipse(ctx); })
        .install();

    seq = 0;
    action(ID_TOOL_FILL, "tools", "fill", seq++, "&Fill", "Fill tool")
        .icon(wxART_TIP, dir, "swatch.svg")
        .no_tool()
        .performFn([this](PerformContext* ctx) { onToolFill(ctx); })
        .install();
    action(ID_TOOL_PICKER, "tools", "picker", seq++, "Pick &Color", "Color picker tool")
        .icon(wxART_FIND, dir, "eye-dropper.svg")
        .no_tool()
        .performFn([this](PerformContext* ctx) { onToolPicker(ctx); })
        .install();
    action(ID_TOOL_TEXT, "tools", "text", seq++, "&Text", "Text tool")
        .icon(wxART_TIP, dir, "chat-bubble-bottom-center-text.svg")
        .no_tool()
        .performFn([this](PerformContext* ctx) { onToolText(ctx); })
        .install();
}

void PaintBody::loadImage(const wxImage& img) {
    if (m_canvas) {
        m_canvas->loadImage(img);
        return;
    }
    m_pendingImage = img;
}

void PaintBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    m_root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    wxBoxSizer* rootSizer = new wxBoxSizer(wxHORIZONTAL);

    m_toolToggles.clear();

    wxPanel* toolsPanel = new wxPanel(m_root, wxID_ANY);
    toolsPanel->SetMinSize(wxSize(88, -1));
    wxBoxSizer* toolsSizer = new wxBoxSizer(wxVERTICAL);

    const std::string iconDir = "heroicons/normal";
    constexpr int kIconPx = 24;
    auto toolBmp = [&](wxArtID art, const char* svg) {
        return ImageSet(art, iconDir, svg).toBitmap1(kIconPx, kIconPx);
    };

    struct ToolSpec {
        wxArtID art;
        const char* svg;
        const char* tip;
        PaintTool tool;
    };
    static const ToolSpec kToolSpecs[] = {
        {wxART_TIP, "pencil.svg", "Pencil", PaintTool::Pencil},
        {wxART_TIP, "paint-brush.svg", "Brush", PaintTool::Brush},
        {wxART_DELETE, "backspace.svg", "Eraser", PaintTool::Eraser},
        {wxART_MINUS, "minus.svg", "Line", PaintTool::Line},
        {wxART_NORMAL_FILE, "rectangle-group.svg", "Rectangle", PaintTool::Rect},
        {wxART_NORMAL_FILE, "stop-circle.svg", "Ellipse", PaintTool::Ellipse},
        {wxART_TIP, "swatch.svg", "Fill", PaintTool::Fill},
        {wxART_FIND, "eye-dropper.svg", "Pick color", PaintTool::Picker},
        {wxART_TIP, "chat-bubble-bottom-center-text.svg", "Text", PaintTool::Text},
    };

    wxBoxSizer* toolsRow = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* iconCol = new wxBoxSizer(wxVERTICAL);
    for (const ToolSpec& spec : kToolSpecs) {
        wxBitmap bm = toolBmp(spec.art, spec.svg);
        auto* b = new wxBitmapToggleButton(toolsPanel, wxID_ANY, bm, wxDefaultPosition, wxDefaultSize,
                                           wxBU_AUTODRAW | wxBORDER_NONE);
        b->SetToolTip(spec.tip);
        m_toolToggles.push_back({b, spec.tool});
        iconCol->Add(b, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 3);
        b->Bind(wxEVT_TOGGLEBUTTON, [this, b, t = spec.tool](wxCommandEvent&) {
            if (!b->GetValue()) {
                b->SetValue(true);
                return;
            }
            setCurrentTool(t);
        });
    }
    toolsRow->Add(iconCol, 0, wxEXPAND | wxTOP | wxBOTTOM | wxLEFT, 6);

    wxBoxSizer* sliderCol = new wxBoxSizer(wxVERTICAL);
    wxSlider* width = new wxSlider(toolsPanel, wxID_ANY, 3, 1, 30, wxDefaultPosition, wxSize(-1, 200),
                                   wxSL_VERTICAL | wxSL_INVERSE);
    width->SetToolTip("Stroke width");
    sliderCol->Add(width, 1, wxEXPAND | wxTOP | wxBOTTOM, 6);
    toolsRow->Add(sliderCol, 0, wxEXPAND | wxRIGHT | wxTOP | wxBOTTOM, 8);

    toolsSizer->Add(toolsRow, 1, wxEXPAND);

    wxBitmap bmpClear = ImageSet(wxART_DELETE, iconDir, "trash.svg").toBitmap1(kIconPx, kIconPx);
    wxBitmap bmpSave = ImageSet(wxART_FILE_SAVE_AS, iconDir, "arrow-down-tray.svg").toBitmap1(kIconPx, kIconPx);
    auto* clearBtn = new wxBitmapButton(toolsPanel, wxID_ANY, bmpClear, wxDefaultPosition, wxDefaultSize,
                                        wxBU_AUTODRAW | wxBORDER_NONE);
    clearBtn->SetToolTip("Clear canvas");
    auto* saveBtn = new wxBitmapButton(toolsPanel, wxID_ANY, bmpSave, wxDefaultPosition, wxDefaultSize,
                                       wxBU_AUTODRAW | wxBORDER_NONE);
    saveBtn->SetToolTip("Save PNG...");
    wxBoxSizer* fileRow = new wxBoxSizer(wxHORIZONTAL);
    fileRow->Add(clearBtn, 0, wxRIGHT, 6);
    fileRow->Add(saveBtn, 0);
    toolsSizer->Add(fileRow, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);
    toolsPanel->SetSizer(toolsSizer);

    wxPanel* rightPanel = new wxPanel(m_root, wxID_ANY);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    m_canvas = new PaintCanvas(rightPanel);
    if (m_pendingImage.IsOk()) {
        m_canvas->loadImage(m_pendingImage);
        m_pendingImage = wxImage();
    }

    wxPanel* palettePanel = new wxPanel(rightPanel, wxID_ANY);
    palettePanel->SetMinSize(wxSize(-1, 64));
    wxBoxSizer* paletteSizer = new wxBoxSizer(wxHORIZONTAL);
    paletteSizer->Add(new wxStaticText(palettePanel, wxID_ANY, "Palette:"), 0,
                      wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);

    auto addColor = [palettePanel, this, &paletteSizer](const wxColour& c) {
        wxButton* b = new wxButton(palettePanel, wxID_ANY, "", wxDefaultPosition, wxSize(22, 22));
        b->SetBackgroundColour(c);
        b->Bind(wxEVT_BUTTON, [this, c](wxCommandEvent&) {
            if (m_canvas)
                m_canvas->setPrimaryColor(c);
        });
        paletteSizer->Add(b, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    };

    addColor(*wxBLACK);
    addColor(wxColour(0x80, 0x80, 0x80));
    addColor(*wxWHITE);
    addColor(*wxRED);
    addColor(*wxGREEN);
    addColor(*wxBLUE);
    addColor(wxColour(0xff, 0xff, 0x00));
    addColor(wxColour(0xff, 0xa5, 0x00));
    addColor(wxColour(0x80, 0x00, 0x80));
    addColor(wxColour(0x00, 0xff, 0xff));
    paletteSizer->AddStretchSpacer();
    palettePanel->SetSizer(paletteSizer);

    rightSizer->Add(m_canvas, 1, wxEXPAND | wxALL, 6);
    rightSizer->Add(palettePanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    rightPanel->SetSizer(rightSizer);

    rootSizer->Add(toolsPanel, 0, wxEXPAND);
    rootSizer->Add(rightPanel, 1, wxEXPAND);
    m_root->SetSizer(rootSizer);

    setCurrentTool(PaintTool::Pencil);

    width->Bind(wxEVT_SLIDER, [this, width](wxCommandEvent&) {
        if (m_canvas)
            m_canvas->setWidth(width->GetValue());
    });
    clearBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        auto ctx2 = toPerformContext(e);
        onClear(&ctx2);
    });
    saveBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        auto ctx2 = toPerformContext(e);
        onSavePng(&ctx2);
    });
}

wxEvtHandler* PaintBody::getEventHandler() {
    return m_root ? m_root->GetEventHandler() : nullptr;
}

void PaintBody::onSavePng(PerformContext*) {
    if (!m_frame || !m_canvas)
        return;
    wxFileDialog dlg(m_frame, "Save image", "", "paint.png", "PNG files (*.png)|*.png",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) {
        m_canvas->savePng(dlg.GetPath());
    }
}

void PaintBody::onClear(PerformContext*) {
    if (m_canvas)
        m_canvas->clear();
}

void PaintBody::setCurrentTool(PaintTool t) {
    if (m_canvas)
        m_canvas->setTool(t);
    for (auto& [btn, tool] : m_toolToggles) {
        if (btn)
            btn->SetValue(tool == t);
    }
}

void PaintBody::onToolPencil(PerformContext*) { setCurrentTool(PaintTool::Pencil); }
void PaintBody::onToolBrush(PerformContext*) { setCurrentTool(PaintTool::Brush); }
void PaintBody::onToolEraser(PerformContext*) { setCurrentTool(PaintTool::Eraser); }
void PaintBody::onToolLine(PerformContext*) { setCurrentTool(PaintTool::Line); }
void PaintBody::onToolRect(PerformContext*) { setCurrentTool(PaintTool::Rect); }
void PaintBody::onToolEllipse(PerformContext*) { setCurrentTool(PaintTool::Ellipse); }
void PaintBody::onToolFill(PerformContext*) { setCurrentTool(PaintTool::Fill); }
void PaintBody::onToolPicker(PerformContext*) { setCurrentTool(PaintTool::Picker); }
void PaintBody::onToolText(PerformContext*) { setCurrentTool(PaintTool::Text); }

} // namespace os


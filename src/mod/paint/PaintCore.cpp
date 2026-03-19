#include "PaintCore.hpp"

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

enum class Tool {
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

class PaintCore::PaintCanvas : public wxPanel {
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

    void setPrimaryColor(const wxColour& c) { primary_ = c; }
    void setWidth(int w) { width_ = std::max(1, w); }
    void setTool(Tool t) { tool_ = t; }

    void clear() {
        ensureBuffer();
        wxMemoryDC dc(buffer_);
        dc.SetBackground(wxBrush(*wxWHITE));
        dc.Clear();
        previewActive_ = false;
        Refresh();
    }

    bool savePng(const wxString& path) {
        ensureBuffer();
        wxImage img = buffer_.ConvertToImage();
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
        buffer_ = wxBitmap(scaled);
        previewActive_ = false;
        Refresh();
    }

private:
    void ensureBuffer() {
        wxSize sz = GetClientSize();
        if (sz.GetWidth() <= 0 || sz.GetHeight() <= 0)
            return;
        if (!buffer_.IsOk() || buffer_.GetWidth() != sz.GetWidth() || buffer_.GetHeight() != sz.GetHeight()) {
            wxBitmap newBuf(sz.GetWidth(), sz.GetHeight());
            wxMemoryDC dc(newBuf);
            dc.SetBackground(wxBrush(*wxWHITE));
            dc.Clear();
            if (buffer_.IsOk()) {
                wxMemoryDC old(buffer_);
                dc.Blit(0, 0, buffer_.GetWidth(), buffer_.GetHeight(), &old, 0, 0);
            }
            buffer_ = newBuf;
        }
    }

    wxPen makePen(bool erasing) const {
        wxPen pen(erasing ? *wxWHITE : primary_, width_, wxPENSTYLE_SOLID);
        pen.SetCap(wxCAP_ROUND);
        pen.SetJoin(wxJOIN_ROUND);
        return pen;
    }

    void drawStroke(const wxPoint& a, const wxPoint& b, bool erasing) {
        ensureBuffer();
        wxMemoryDC dc(buffer_);
        dc.SetPen(makePen(erasing));
        dc.DrawLine(a, b);
    }

    void drawShapeTo(wxDC& dc, Tool shape, const wxPoint& a, const wxPoint& b) {
        wxPen pen = makePen(false);
        dc.SetPen(pen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        if (shape == Tool::Line) {
            dc.DrawLine(a, b);
            return;
        }
        int x = std::min(a.x, b.x);
        int y = std::min(a.y, b.y);
        int w = std::abs(a.x - b.x);
        int h = std::abs(a.y - b.y);
        if (shape == Tool::Rect) {
            dc.DrawRectangle(x, y, w, h);
        } else if (shape == Tool::Ellipse) {
            dc.DrawEllipse(x, y, w, h);
        }
    }

    static bool colorEquals(const wxImage& img, int x, int y, const wxColour& c) {
        return img.GetRed(x, y) == c.Red() && img.GetGreen(x, y) == c.Green() && img.GetBlue(x, y) == c.Blue();
    }

    void floodFill(const wxPoint& pt, const wxColour& fillColor) {
        ensureBuffer();
        if (!buffer_.IsOk())
            return;
        wxImage img = buffer_.ConvertToImage();
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
        buffer_ = wxBitmap(img);
        previewActive_ = false;
        Refresh(false);
    }

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(*wxWHITE));
        dc.Clear();
        if (buffer_.IsOk()) {
            dc.DrawBitmap(buffer_, 0, 0, false);
        }
        if (previewActive_ && (tool_ == Tool::Line || tool_ == Tool::Rect || tool_ == Tool::Ellipse)) {
            drawShapeTo(dc, tool_, dragStart_, last_);
        }
    }

    void OnLeftDown(wxMouseEvent& e) {
        ensureBuffer();
        CaptureMouse();
        dragging_ = true;
        dragStart_ = e.GetPosition();
        last_ = dragStart_;

        if (tool_ == Tool::Picker) {
            if (buffer_.IsOk()) {
                wxImage img = buffer_.ConvertToImage();
                if (img.IsOk() && dragStart_.x >= 0 && dragStart_.y >= 0 &&
                    dragStart_.x < img.GetWidth() && dragStart_.y < img.GetHeight()) {
                    primary_ = wxColour(img.GetRed(dragStart_.x, dragStart_.y),
                                        img.GetGreen(dragStart_.x, dragStart_.y),
                                        img.GetBlue(dragStart_.x, dragStart_.y));
                }
            }
            dragging_ = false;
            if (HasCapture())
                ReleaseMouse();
            return;
        }

        if (tool_ == Tool::Fill) {
            floodFill(dragStart_, primary_);
            dragging_ = false;
            if (HasCapture())
                ReleaseMouse();
            return;
        }

        if (tool_ == Tool::Text) {
            wxTextEntryDialog dlg(this, "Text:", "Paint");
            if (dlg.ShowModal() == wxID_OK) {
                wxString text = dlg.GetValue();
                if (!text.IsEmpty()) {
                    wxMemoryDC dc(buffer_);
                    dc.SetTextForeground(primary_);
                    dc.DrawText(text, dragStart_);
                    Refresh(false);
                }
            }
            dragging_ = false;
            if (HasCapture())
                ReleaseMouse();
            return;
        }

        previewActive_ = (tool_ == Tool::Line || tool_ == Tool::Rect || tool_ == Tool::Ellipse);
    }

    void OnLeftUp(wxMouseEvent&) {
        if (previewActive_ && (tool_ == Tool::Line || tool_ == Tool::Rect || tool_ == Tool::Ellipse)) {
            ensureBuffer();
            wxMemoryDC dc(buffer_);
            drawShapeTo(dc, tool_, dragStart_, last_);
            previewActive_ = false;
            Refresh(false);
        }
        dragging_ = false;
        if (HasCapture())
            ReleaseMouse();
    }

    void OnMove(wxMouseEvent& e) {
        if (!dragging_ || !e.LeftIsDown())
            return;
        wxPoint p = e.GetPosition();
        if (tool_ == Tool::Pencil) {
            drawStroke(last_, p, false);
        } else if (tool_ == Tool::Brush) {
            int oldW = width_;
            width_ = std::max(width_, 6);
            drawStroke(last_, p, false);
            width_ = oldW;
        } else if (tool_ == Tool::Eraser) {
            drawStroke(last_, p, true);
        } else if (tool_ == Tool::Line || tool_ == Tool::Rect || tool_ == Tool::Ellipse) {
            previewActive_ = true;
        }
        last_ = p;
        Refresh(false);
    }

    void OnSize(wxSizeEvent& e) {
        ensureBuffer();
        e.Skip();
        Refresh();
    }

    wxBitmap buffer_;
    wxColour primary_{0, 0, 0};
    int width_{3};
    Tool tool_{Tool::Pencil};
    bool dragging_{false};
    wxPoint last_{0, 0};
    wxPoint dragStart_{0, 0};
    bool previewActive_{false};
};

PaintCore::PaintCore() {
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
        .performFn([this](PerformContext* ctx) { onToolPencil(ctx); })
        .install();
    action(ID_TOOL_BRUSH, "tools/draw", "brush", seq++, "&Brush", "Brush tool")
        .icon(wxART_TIP, dir, "paint-brush.svg")
        .performFn([this](PerformContext* ctx) { onToolBrush(ctx); })
        .install();
    action(ID_TOOL_ERASER, "tools/draw", "eraser", seq++, "&Eraser", "Eraser tool")
        .icon(wxART_DELETE, dir, "backspace.svg")
        .performFn([this](PerformContext* ctx) { onToolEraser(ctx); })
        .install();

    seq = 0;
    action(ID_TOOL_LINE, "tools/shapes", "line", seq++, "&Line", "Line tool")
        .icon(wxART_MINUS, dir, "minus.svg")
        .performFn([this](PerformContext* ctx) { onToolLine(ctx); })
        .install();
    action(ID_TOOL_RECT, "tools/shapes", "rect", seq++, "&Rectangle", "Rectangle tool")
        .icon(wxART_NORMAL_FILE, dir, "rectangle-group.svg")
        .performFn([this](PerformContext* ctx) { onToolRect(ctx); })
        .install();
    action(ID_TOOL_ELLIPSE, "tools/shapes", "ellipse", seq++, "&Ellipse", "Ellipse tool")
        .icon(wxART_NORMAL_FILE, dir, "stop-circle.svg")
        .performFn([this](PerformContext* ctx) { onToolEllipse(ctx); })
        .install();

    seq = 0;
    action(ID_TOOL_FILL, "tools", "fill", seq++, "&Fill", "Fill tool")
        .icon(wxART_TIP, dir, "swatch.svg")
        .performFn([this](PerformContext* ctx) { onToolFill(ctx); })
        .install();
    action(ID_TOOL_PICKER, "tools", "picker", seq++, "Pick &Color", "Color picker tool")
        .icon(wxART_FIND, dir, "eye-dropper.svg")
        .performFn([this](PerformContext* ctx) { onToolPicker(ctx); })
        .install();
    action(ID_TOOL_TEXT, "tools", "text", seq++, "&Text", "Text tool")
        .icon(wxART_TIP, dir, "chat-bubble-bottom-center-text.svg")
        .performFn([this](PerformContext* ctx) { onToolText(ctx); })
        .install();
}

void PaintCore::loadImage(const wxImage& img) {
    if (canvas_) {
        canvas_->loadImage(img);
        return;
    }
    pendingImage_ = img;
}

void PaintCore::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    frame_ = frame;

    root_ = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    wxBoxSizer* rootSizer = new wxBoxSizer(wxHORIZONTAL);

    wxPanel* toolsPanel = new wxPanel(root_, wxID_ANY);
    toolsPanel->SetMinSize(wxSize(140, -1));
    wxBoxSizer* toolsSizer = new wxBoxSizer(wxVERTICAL);
    toolsSizer->Add(new wxStaticText(toolsPanel, wxID_ANY, "Tools"), 0, wxLEFT | wxTOP | wxBOTTOM, 8);

    auto makeToolBtn = [toolsPanel](const wxString& text) {
        return new wxToggleButton(toolsPanel, wxID_ANY, text, wxDefaultPosition, wxSize(120, 28));
    };

    wxToggleButton* btnPencil = makeToolBtn("Pencil");
    wxToggleButton* btnBrush = makeToolBtn("Brush");
    wxToggleButton* btnEraser = makeToolBtn("Eraser");
    wxToggleButton* btnLine = makeToolBtn("Line");
    wxToggleButton* btnRect = makeToolBtn("Rectangle");
    wxToggleButton* btnEllipse = makeToolBtn("Ellipse");
    wxToggleButton* btnFill = makeToolBtn("Fill");
    wxToggleButton* btnPicker = makeToolBtn("Pick Color");
    wxToggleButton* btnText = makeToolBtn("Text");

    std::vector<std::pair<wxToggleButton*, Tool>> toolButtons = {
        {btnPencil, Tool::Pencil}, {btnBrush, Tool::Brush},   {btnEraser, Tool::Eraser},
        {btnLine, Tool::Line},     {btnRect, Tool::Rect},     {btnEllipse, Tool::Ellipse},
        {btnFill, Tool::Fill},     {btnPicker, Tool::Picker}, {btnText, Tool::Text},
    };

    for (auto& it : toolButtons) {
        toolsSizer->Add(it.first, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    }

    toolsSizer->AddStretchSpacer();
    toolsSizer->Add(new wxStaticText(toolsPanel, wxID_ANY, "Width"), 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
    wxSlider* width = new wxSlider(toolsPanel, wxID_ANY, 3, 1, 30, wxDefaultPosition, wxSize(120, -1));
    toolsSizer->Add(width, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    wxButton* clearBtn = new wxButton(toolsPanel, wxID_ANY, "Clear");
    wxButton* saveBtn = new wxButton(toolsPanel, wxID_ANY, "Save PNG...");
    toolsSizer->Add(clearBtn, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    toolsSizer->Add(saveBtn, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    toolsPanel->SetSizer(toolsSizer);

    wxPanel* rightPanel = new wxPanel(root_, wxID_ANY);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    canvas_ = new PaintCanvas(rightPanel);
    if (pendingImage_.IsOk()) {
        canvas_->loadImage(pendingImage_);
        pendingImage_ = wxImage();
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
            if (canvas_)
                canvas_->setPrimaryColor(c);
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

    rightSizer->Add(canvas_, 1, wxEXPAND | wxALL, 6);
    rightSizer->Add(palettePanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    rightPanel->SetSizer(rightSizer);

    rootSizer->Add(toolsPanel, 0, wxEXPAND);
    rootSizer->Add(rightPanel, 1, wxEXPAND);
    root_->SetSizer(rootSizer);

    auto setToolSelected = [this, &toolButtons](Tool t) {
        if (canvas_)
            canvas_->setTool(t);
        for (auto& [btn, tool] : toolButtons) {
            btn->SetValue(tool == t);
        }
    };
    setToolSelected(Tool::Pencil);

    for (auto& [btn, tool] : toolButtons) {
        btn->Bind(wxEVT_TOGGLEBUTTON, [setToolSelected, tool](wxCommandEvent&) { setToolSelected(tool); });
    }

    width->Bind(wxEVT_SLIDER, [this, width](wxCommandEvent&) {
        if (canvas_)
            canvas_->setWidth(width->GetValue());
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

wxEvtHandler* PaintCore::getEventHandler() {
    return root_ ? root_->GetEventHandler() : nullptr;
}

void PaintCore::onSavePng(PerformContext*) {
    if (!frame_ || !canvas_)
        return;
    wxFileDialog dlg(frame_, "Save image", "", "paint.png", "PNG files (*.png)|*.png",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) {
        canvas_->savePng(dlg.GetPath());
    }
}

void PaintCore::onClear(PerformContext*) {
    if (canvas_)
        canvas_->clear();
}

void PaintCore::onToolPencil(PerformContext*) { if (canvas_) canvas_->setTool(Tool::Pencil); }
void PaintCore::onToolBrush(PerformContext*) { if (canvas_) canvas_->setTool(Tool::Brush); }
void PaintCore::onToolEraser(PerformContext*) { if (canvas_) canvas_->setTool(Tool::Eraser); }
void PaintCore::onToolLine(PerformContext*) { if (canvas_) canvas_->setTool(Tool::Line); }
void PaintCore::onToolRect(PerformContext*) { if (canvas_) canvas_->setTool(Tool::Rect); }
void PaintCore::onToolEllipse(PerformContext*) { if (canvas_) canvas_->setTool(Tool::Ellipse); }
void PaintCore::onToolFill(PerformContext*) { if (canvas_) canvas_->setTool(Tool::Fill); }
void PaintCore::onToolPicker(PerformContext*) { if (canvas_) canvas_->setTool(Tool::Picker); }
void PaintCore::onToolText(PerformContext*) { if (canvas_) canvas_->setTool(Tool::Text); }

} // namespace os


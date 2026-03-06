#include "DesktopWindow.hpp"

#include "../core/ModuleRegistry.hpp"

#include <bas/proc/wx_assets.hpp>
#include <bas/ui/arch/ImageSet.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/log.h>

#include <algorithm>

namespace os {

BEGIN_EVENT_TABLE(DesktopWindow, wxPanel)
    EVT_PAINT(DesktopWindow::OnPaint)
    EVT_SIZE(DesktopWindow::OnSize)
    EVT_LEFT_DCLICK(DesktopWindow::OnLeftDoubleClick)
    EVT_RIGHT_DOWN(DesktopWindow::OnRightClick)
    EVT_ERASE_BACKGROUND(DesktopWindow::OnEraseBackground)
END_EVENT_TABLE()

DesktopWindow::DesktopWindow(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
    , volumeManager_(nullptr)
    , backgroundColor_(*wxLIGHT_GREY)
    , iconSize_(64, 80)
    , iconSpacing_(10)
    , margin_(20)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(800, 600));
}

DesktopWindow::~DesktopWindow() {
    clearIcons();
}

void DesktopWindow::addIcon(ModulePtr module, const wxPoint& position) {
    if (!module) return;
    
    // Check if icon already exists
    if (FindIconByModule(module->getFullUri())) {
        wxLogWarning("Icon already exists for module: %s", module->getFullUri());
        return;
    }
    
    DesktopIcon icon;
    icon.module = module;
    icon.position = position;
    icon.selected = false;
    
    CreateIconControls(icon);
    icons_.push_back(icon);
    
    Refresh();
}

void DesktopWindow::removeIcon(const std::string& moduleUri) {
    auto it = std::find_if(icons_.begin(), icons_.end(),
        [&moduleUri](const DesktopIcon& icon) {
            return icon.module && icon.module->getFullUri() == moduleUri;
        });
    
    if (it != icons_.end()) {
        // Remove controls
        if (it->bitmap) {
            it->bitmap->Destroy();
        }
        if (it->label) {
            it->label->Destroy();
        }
        icons_.erase(it);
        Refresh();
    }
}

void DesktopWindow::setVolumeManager(VolumeManager* vm) {
    volumeManager_ = vm;
}

void DesktopWindow::addVolumeIcons() {
    if (!volumeManager_) return;
    for (size_t i = 0; i < volumeManager_->getVolumeCount(); i++) {
        Volume* vol = volumeManager_->getVolume(i);
        if (!vol) continue;
        VolumeDesktopIcon icon;
        icon.volume = vol;
        icon.position = wxPoint(0, 0);
        CreateVolumeIconControls(icon);
        volumeIcons_.push_back(icon);
    }
    Refresh();
}

void DesktopWindow::clearIcons() {
    for (auto& icon : volumeIcons_) {
        if (icon.bitmap) icon.bitmap->Destroy();
        if (icon.label) icon.label->Destroy();
    }
    volumeIcons_.clear();
    for (auto& icon : icons_) {
        if (icon.bitmap) icon.bitmap->Destroy();
        if (icon.label) icon.label->Destroy();
    }
    icons_.clear();
}

void DesktopWindow::arrangeIcons() {
    int x = margin_;
    int y = margin_;
    int maxWidth = GetClientSize().GetWidth();

    for (auto& icon : volumeIcons_) {
        if (x + iconSize_.GetWidth() > maxWidth - margin_) {
            x = margin_;
            y += iconSize_.GetHeight() + iconSpacing_;
        }
        icon.position = wxPoint(x, y);
        if (icon.bitmap) icon.bitmap->Move(x, y);
        if (icon.label) icon.label->Move(x, y + iconSize_.GetWidth() + 2);
        x += iconSize_.GetWidth() + iconSpacing_;
    }
    for (auto& icon : icons_) {
        if (x + iconSize_.GetWidth() > maxWidth - margin_) {
            x = margin_;
            y += iconSize_.GetHeight() + iconSpacing_;
        }
        icon.position = wxPoint(x, y);
        if (icon.bitmap) icon.bitmap->Move(x, y);
        if (icon.label) icon.label->Move(x, y + iconSize_.GetWidth() + 2);
        x += iconSize_.GetWidth() + iconSpacing_;
    }
    Refresh();
}

void DesktopWindow::CreateVolumeIconControls(VolumeDesktopIcon& icon) {
    if (!icon.volume) return;

    ImageSet iconSet = ImageSet(Path("streamline-vectors/core/pop/computer-devices/hard-disk.svg"));
    wxBitmap bitmap = iconSet.loadBitmap(32, 32);

    if (!bitmap.IsOk()) {
        bitmap = wxArtProvider::GetBitmap(wxART_HARDDISK, wxART_MENU, wxSize(32, 32));
    }
    icon.bitmap = new wxStaticBitmap(this, wxID_ANY, bitmap, icon.position, wxSize(32, 32));
    std::string labelStr = icon.volume->getLabel();
    if (labelStr.empty()) labelStr = icon.volume->getId();
    icon.label = new wxStaticText(this, wxID_ANY, labelStr,
                                  wxPoint(icon.position.x, icon.position.y + 34),
                                  wxSize(60, 20), wxALIGN_CENTRE);
    icon.label->Wrap(60);
}

void DesktopWindow::setBackgroundColor(const wxColour& color) {
    backgroundColor_ = color;
    Refresh();
}

void DesktopWindow::setBackgroundImage(const wxBitmap& bitmap) {
    backgroundImage_ = bitmap;
    Refresh();
}

void DesktopWindow::launchModule(ModulePtr module) {
    if (!module || !module->isEnabled()) {
        wxLogWarning("Cannot launch module: not enabled");
        return;
    }
    
    try {
        module->recordExecution();
        module->run();
    } catch (const std::exception& e) {
        wxLogError("Failed to launch module %s: %s", 
                   module->getFullUri(), e.what());
        wxMessageBox("Failed to launch " + module->label + ": " + e.what(),
                     "Error", wxOK | wxICON_ERROR);
    }
}

void DesktopWindow::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    
    wxSize size = GetClientSize();
    
    // Draw background
    if (backgroundImage_.IsOk()) {
        // Tile or stretch background image
        dc.DrawBitmap(backgroundImage_, 0, 0, false);
    } else {
        dc.SetBackground(wxBrush(backgroundColor_));
        dc.Clear();
    }
    
    event.Skip();
}

void DesktopWindow::OnSize(wxSizeEvent& event) {
    arrangeIcons();
    event.Skip();
}

void DesktopWindow::OnLeftDoubleClick(wxMouseEvent& event) {
    // Check if double-clicked on an icon
    wxPoint pos = event.GetPosition();
    DesktopIcon* icon = FindIconAt(pos);
    
    if (icon && icon->module) {
        launchModule(icon->module);
    }
    
    event.Skip();
}

void DesktopWindow::OnRightClick(wxMouseEvent& event) {
    // Desktop context menu
    wxMenu menu;
    menu.Append(wxID_ANY, "Refresh Desktop");
    menu.Append(wxID_ANY, "Arrange Icons");
    menu.AppendSeparator();
    menu.Append(wxID_ANY, "Properties");
    
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        clearIcons();
        addVolumeIcons();
        auto& registry = ModuleRegistry::getInstance();
        for (auto& module : registry.getVisibleModules()) {
            addIcon(module);
        }
        arrangeIcons();
    }, menu.FindItemByPosition(0)->GetId());
    
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        arrangeIcons();
    }, menu.FindItemByPosition(1)->GetId());
    
    PopupMenu(&menu, event.GetPosition());
    
    event.Skip();
}

void DesktopWindow::OnEraseBackground(wxEraseEvent& event) {
    // Prevent flicker
    event.Skip();
}

void DesktopWindow::CreateIconControls(DesktopIcon& icon) {
    if (!icon.module) return;
    
    // Create icon bitmap
    wxBitmap bitmap;
    if (icon.module->image.loadBitmap(32, 32).IsOk()) {
        bitmap = icon.module->image.loadBitmap(32, 32);
    } else {
        // Default icon
        bitmap = wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, wxART_MENU, wxSize(32, 32));
    }
    
    icon.bitmap = new wxStaticBitmap(this, wxID_ANY, bitmap, 
                                      icon.position, wxSize(32, 32));
    
    // Create label
    icon.label = new wxStaticText(this, wxID_ANY, icon.module->label,
                                   wxPoint(icon.position.x, icon.position.y + 34),
                                   wxSize(60, 20), wxALIGN_CENTRE);
    icon.label->Wrap(60);
    
    // Bind events
    icon.bitmap->Bind(wxEVT_LEFT_DCLICK, &DesktopWindow::OnIconLeftDoubleClick, this);
    icon.label->Bind(wxEVT_LEFT_DCLICK, &DesktopWindow::OnIconLeftDoubleClick, this);
    icon.bitmap->Bind(wxEVT_RIGHT_DOWN, &DesktopWindow::OnIconRightClick, this);
    icon.label->Bind(wxEVT_RIGHT_DOWN, &DesktopWindow::OnIconRightClick, this);
}

void DesktopWindow::OnIconLeftDoubleClick(wxMouseEvent& event) {
    event.Skip();
}

void DesktopWindow::OnIconRightClick(wxMouseEvent& event) {
    // Find which icon was clicked
    wxWindow* child = FindFocus();
    DesktopIcon* clickedIcon = nullptr;
    
    for (auto& icon : icons_) {
        if (icon.bitmap == child || icon.label == child) {
            clickedIcon = &icon;
            break;
        }
    }
    
    if (!clickedIcon || !clickedIcon->module) {
        event.Skip();
        return;
    }
    
    // Create context menu
    wxMenu menu;
    menu.Append(wxID_ANY, "Open");
    menu.Append(wxID_ANY, "Run");
    menu.AppendSeparator();
    menu.Append(wxID_ANY, "Properties");
    menu.Append(wxID_ANY, "Remove from Desktop");
    
    ModulePtr module = clickedIcon->module;
    
    Bind(wxEVT_MENU, [this, module](wxCommandEvent&) {
        launchModule(module);
    }, menu.FindItemByPosition(0)->GetId());
    
    Bind(wxEVT_MENU, [this, module](wxCommandEvent&) {
        launchModule(module);
    }, menu.FindItemByPosition(1)->GetId());
    
    PopupMenu(&menu, event.GetPosition());
    
    event.Skip();
}

DesktopIcon* DesktopWindow::FindIconAt(const wxPoint& pos) {
    for (auto& icon : icons_) {
        if (icon.bitmap) {
            wxRect rect = icon.bitmap->GetRect();
            if (rect.Contains(pos)) {
                return &icon;
            }
        }
    }
    return nullptr;
}

DesktopIcon* DesktopWindow::FindIconByModule(const std::string& uri) {
    for (auto& icon : icons_) {
        if (icon.module && icon.module->getFullUri() == uri) {
            return &icon;
        }
    }
    return nullptr;
}

} // namespace os

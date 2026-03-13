#include "DesktopWindow.hpp"

#include "../core/ModuleRegistry.hpp"

#include <bas/proc/wx_assets.hpp>
#include <bas/ui/arch/ImageSet.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/log.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

std::string getProjectName() {
    if (wxTheApp) {
        return wxTheApp->GetAppName().ToStdString();
    }
    return "omnishell";
}

std::filesystem::path getLayoutPath() {
    namespace fs = std::filesystem;
    const char* home = std::getenv("HOME");
    fs::path base = home ? fs::path(home) : fs::path{};
    fs::path dir = base / ".cache" / getProjectName();
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir / "desktop.json";
}

std::string escapeJson(const std::string& in) {
    std::string out;
    out.reserve(in.size() + 8);
    for (char c : in) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
        }
    }
    return out;
}

} // namespace

namespace os {

BEGIN_EVENT_TABLE(DesktopWindow, wxPanel)
EVT_PAINT(DesktopWindow::OnPaint)
EVT_SIZE(DesktopWindow::OnSize)
EVT_LEFT_DCLICK(DesktopWindow::OnLeftDoubleClick)
EVT_RIGHT_DOWN(DesktopWindow::OnRightClick)
EVT_ERASE_BACKGROUND(DesktopWindow::OnEraseBackground)
END_EVENT_TABLE()

DesktopWindow::DesktopWindow(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), volumeManager_(nullptr), backgroundColor_(*wxLIGHT_GREY),
      iconSize_(64, 80), iconSpacing_(10), margin_(20) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(800, 600));
}

DesktopWindow::~DesktopWindow() { clearIcons(); }

void DesktopWindow::addIcon(ModulePtr module, const wxPoint& position) {
    if (!module)
        return;

    // Check if icon already exists
    if (FindIconByModule(module->getFullUri())) {
        wxLogWarning("Icon already exists for module: %s", module->getFullUri());
        return;
    }

    DesktopModuleIcon icon;
    icon.module = module;
    icon.position = position;
    icon.selected = false;

    CreateIconControls(icon);
    icons_.push_back(icon);

    Refresh();
}

void DesktopWindow::removeIcon(const std::string& moduleUri) {
    auto it = std::find_if(
        icons_.begin(), icons_.end(),
        [&moduleUri](const DesktopModuleIcon& icon) {
            return icon.module && icon.module->getFullUri() == moduleUri;
        });

    if (it != icons_.end()) {
        if (it->widget) {
            it->widget->Destroy();
        }
        icons_.erase(it);
        Refresh();
    }
}

void DesktopWindow::setVolumeManager(VolumeManager* vm) { volumeManager_ = vm; }

void DesktopWindow::addVolumeIcons() {
    if (!volumeManager_)
        return;
    for (size_t i = 0; i < volumeManager_->getVolumeCount(); i++) {
        Volume* vol = volumeManager_->getVolume(i);
        if (!vol)
            continue;
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
        if (icon.widget)
            icon.widget->Destroy();
    }
    volumeIcons_.clear();
    for (auto& icon : icons_) {
        if (icon.widget)
            icon.widget->Destroy();
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
        if (icon.widget) {
            icon.widget->SetPosition(wxPoint(x, y));
        }
        x += iconSize_.GetWidth() + iconSpacing_;
    }
    for (auto& icon : icons_) {
        if (x + iconSize_.GetWidth() > maxWidth - margin_) {
            x = margin_;
            y += iconSize_.GetHeight() + iconSpacing_;
        }
        icon.position = wxPoint(x, y);
        if (icon.widget) {
            icon.widget->SetPosition(wxPoint(x, y));
        }
        x += iconSize_.GetWidth() + iconSpacing_;
    }
    Refresh();
}

void DesktopWindow::CreateVolumeIconControls(VolumeDesktopIcon& icon) {
    if (!icon.volume)
        return;

    std::string dir = "streamline-vectors/core/pop/computer-devices";
    ImageSet iconSet = ImageSet(wxART_HARDDISK, Path(dir, "hard-disk.svg"))
                           .scale(16, 16, Path(dir, "hard-disk-16x16.png"))
                           .scale(24, 24, Path(dir, "hard-disk-24x24.png"))
                           .scale(32, 32, Path(dir, "hard-disk-32x32.png"))
                           .scale(48, 48, Path(dir, "hard-disk-48x48.png"));
    wxBitmap bitmap = iconSet.loadBitmap(32, 32);

    std::string labelStr = icon.volume->getLabel();
    if (labelStr.empty())
        labelStr = icon.volume->getId();
    icon.widget = new LabeledIcon(this, wxID_ANY, bitmap, labelStr, icon.position, iconSize_);

    // Enable dragging for volume icons as well
    icon.widget->Bind(wxEVT_LEFT_DOWN, &DesktopWindow::OnIconLeftDown, this);
    icon.widget->Bind(wxEVT_LEFT_UP, &DesktopWindow::OnIconLeftUp, this);
    icon.widget->Bind(wxEVT_MOTION, &DesktopWindow::OnIconMouseMove, this);
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
        wxLogError("Failed to launch module %s: %s", module->getFullUri(), e.what());
        wxMessageBox("Failed to launch " + module->label + ": " + e.what(), "Error",
                     wxOK | wxICON_ERROR);
    }
}

void DesktopWindow::saveLayout() const {
    namespace fs = std::filesystem;
    fs::path path = getLayoutPath();

    std::ofstream out(path);
    if (!out.is_open()) {
        wxLogWarning("Failed to open desktop layout file for writing: %s",
                     path.string());
        return;
    }

    out << "{\n";

    // Modules
    out << "  \"modules\": [\n";
    for (size_t i = 0; i < icons_.size(); ++i) {
        const auto& icon = icons_[i];
        if (!icon.module)
            continue;
        out << "    {\"uri\": \"" << escapeJson(icon.module->getFullUri()) << "\", "
            << "\"x\": " << icon.position.x << ", "
            << "\"y\": " << icon.position.y << "}";
        if (i + 1 < icons_.size())
            out << ",";
        out << "\n";
    }
    out << "  ],\n";

    // Volumes
    out << "  \"volumes\": [\n";
    for (size_t i = 0; i < volumeIcons_.size(); ++i) {
        const auto& icon = volumeIcons_[i];
        if (!icon.volume)
            continue;
        out << "    {\"id\": \"" << escapeJson(icon.volume->getId()) << "\", "
            << "\"x\": " << icon.position.x << ", "
            << "\"y\": " << icon.position.y << "}";
        if (i + 1 < volumeIcons_.size())
            out << ",";
        out << "\n";
    }
    out << "  ]\n";

    out << "}\n";
}

void DesktopWindow::loadLayout() {
    namespace fs = std::filesystem;
    fs::path path = getLayoutPath();

    std::ifstream in(path);
    if (!in.is_open()) {
        return;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string data = buffer.str();

    auto parseEntries = [&](const std::string& sectionKey,
                            const std::string& idKey,
                            auto&& onEntry) {
        size_t secPos = data.find("\"" + sectionKey + "\"");
        if (secPos == std::string::npos)
            return;
        size_t arrayStart = data.find('[', secPos);
        if (arrayStart == std::string::npos)
            return;
        size_t arrayEnd = data.find(']', arrayStart);
        if (arrayEnd == std::string::npos)
            return;

        size_t pos = arrayStart;
        while (true) {
            size_t idPos = data.find("\"" + idKey + "\"", pos);
            if (idPos == std::string::npos || idPos > arrayEnd)
                break;
            size_t firstQuote = data.find('"', idPos + idKey.size() + 2);
            if (firstQuote == std::string::npos || firstQuote > arrayEnd)
                break;
            size_t secondQuote = data.find('"', firstQuote + 1);
            if (secondQuote == std::string::npos || secondQuote > arrayEnd)
                break;
            std::string id = data.substr(firstQuote + 1,
                                         secondQuote - firstQuote - 1);

            size_t xPos = data.find("\"x\"", secondQuote);
            if (xPos == std::string::npos || xPos > arrayEnd)
                break;
            size_t xColon = data.find(':', xPos);
            size_t xEnd = data.find_first_of(",}", xColon);
            int x = std::stoi(data.substr(xColon + 1, xEnd - xColon - 1));

            size_t yPos = data.find("\"y\"", xEnd);
            if (yPos == std::string::npos || yPos > arrayEnd)
                break;
            size_t yColon = data.find(':', yPos);
            size_t yEnd = data.find_first_of(",}", yColon);
            int y = std::stoi(data.substr(yColon + 1, yEnd - yColon - 1));

            onEntry(id, wxPoint(x, y));

            pos = yEnd;
        }
    };

    // Apply to modules
    parseEntries(
        "modules", "uri",
        [this](const std::string& uri, const wxPoint& pt) {
            for (auto& icon : icons_) {
                if (icon.module && icon.module->getFullUri() == uri) {
                    icon.position = pt;
                    if (icon.widget) {
                        icon.widget->SetPosition(pt);
                    }
                    break;
                }
            }
        });

    // Apply to volumes
    parseEntries(
        "volumes", "id",
        [this](const std::string& id, const wxPoint& pt) {
            for (auto& icon : volumeIcons_) {
                if (icon.volume && icon.volume->getId() == id) {
                    icon.position = pt;
                    if (icon.widget) {
                        icon.widget->SetPosition(pt);
                    }
                    break;
                }
            }
        });
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
    event.Skip();
}

void DesktopWindow::OnLeftDoubleClick(wxMouseEvent& event) {
    // Check if double-clicked on an icon
    wxPoint pos = event.GetPosition();
    DesktopModuleIcon* icon =
        dynamic_cast<DesktopModuleIcon*>(FindIconAt(pos));

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

    Bind(
        wxEVT_MENU,
        [this](wxCommandEvent&) {
            clearIcons();
            addVolumeIcons();
            auto& registry = ModuleRegistry::getInstance();
            for (auto& module : registry.getVisibleModules()) {
                addIcon(module);
            }
            arrangeIcons();
        },
        menu.FindItemByPosition(0)->GetId());

    Bind(
        wxEVT_MENU, [this](wxCommandEvent&) { arrangeIcons(); },
        menu.FindItemByPosition(1)->GetId());

    PopupMenu(&menu, event.GetPosition());

    event.Skip();
}

void DesktopWindow::OnEraseBackground(wxEraseEvent& event) {
    // Prevent flicker
    event.Skip();
}

void DesktopWindow::CreateIconControls(DesktopIcon& icon) {
    auto* moduleIcon = dynamic_cast<DesktopModuleIcon*>(&icon);
    if (!moduleIcon || !moduleIcon->module)
        return;

    // Create icon bitmap
    wxBitmap bitmap;
    wxBitmap tmp = moduleIcon->module->image.loadBitmap(32, 32);
    if (tmp.IsOk()) {
        bitmap = tmp;
    } else {
        // Default icon
        ImageSet executable = ImageSet(wxART_EXECUTABLE_FILE);
        tmp = executable.loadBitmap(32, 32);
        if (tmp.IsOk()) {
            bitmap = tmp;
        }
    }

    icon.widget = new LabeledIcon(this, wxID_ANY, bitmap, //
                                  moduleIcon->module->label, icon.position, iconSize_);

    // Bind events on the composite widget
    icon.widget->onLeftDClick(
        [this](wxMouseEvent& event) { OnIconLeftDoubleClick(event); });
    icon.widget->onRightDown(
        [this](wxMouseEvent& event) { OnIconRightClick(event); });
    icon.widget->Bind(wxEVT_LEFT_DOWN, &DesktopWindow::OnIconLeftDown, this);
    icon.widget->Bind(wxEVT_LEFT_UP, &DesktopWindow::OnIconLeftUp, this);
    icon.widget->Bind(wxEVT_MOTION, &DesktopWindow::OnIconMouseMove, this);
}

void DesktopWindow::OnIconLeftDoubleClick(wxMouseEvent& event) {
    wxWindow* child = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (!child) {
        event.Skip();
        return;
    }

    DesktopModuleIcon* clickedIcon =
        dynamic_cast<DesktopModuleIcon*>(FindIconByWindow(child));
    if (clickedIcon && clickedIcon->module) {
        launchModule(clickedIcon->module);
    }

    event.Skip();
}

void DesktopWindow::OnIconRightClick(wxMouseEvent& event) {
    // Find which icon was clicked
    wxWindow* child = dynamic_cast<wxWindow*>(event.GetEventObject());
    DesktopModuleIcon* clickedIcon =
        dynamic_cast<DesktopModuleIcon*>(FindIconByWindow(child));

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

    Bind(
        wxEVT_MENU, [this, module](wxCommandEvent&) { launchModule(module); },
        menu.FindItemByPosition(0)->GetId());

    Bind(
        wxEVT_MENU, [this, module](wxCommandEvent&) { launchModule(module); },
        menu.FindItemByPosition(1)->GetId());

    PopupMenu(&menu, event.GetPosition());

    event.Skip();
}

void DesktopWindow::OnIconLeftDown(wxMouseEvent& event) {
    wxWindow* window = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (!window) {
        event.Skip();
        return;
    }

    DesktopIcon* icon = FindIconByWindow(window);
    if (!icon || !icon->widget) {
        event.Skip();
        return;
    }

    isDragging_ = true;
    draggingIcon_ = icon;

    // Mouse position in desktop coordinates (regardless of which child generated the event)
    wxPoint localPos = event.GetPosition();           // coords in 'window'
    wxPoint screenPos = window->ClientToScreen(localPos);
    wxPoint desktopPos = ScreenToClient(screenPos);
    dragStartMousePos_ = desktopPos;
    dragStartIconPos_ = icon->position;

    if (!icon->widget->HasCapture()) {
        icon->widget->CaptureMouse();
    }

    event.Skip();
}

void DesktopWindow::OnIconLeftUp(wxMouseEvent& event) {
    wxWindow* window = dynamic_cast<wxWindow*>(event.GetEventObject());

    // Handle end of drag
    if (isDragging_ && draggingIcon_) {
        if (draggingIcon_->widget && draggingIcon_->widget->HasCapture()) {
            draggingIcon_->widget->ReleaseMouse();
        }

        isDragging_ = false;
        draggingIcon_ = nullptr;

        // Persist new layout after drag ends
        saveLayout();
    }

    // Handle "open on second button release" for double-click
    if (event.ButtonDClick()) {
        if (window) {
            DesktopModuleIcon* icon =
                dynamic_cast<DesktopModuleIcon*>(FindIconByWindow(window));
            if (icon && icon->module) {
                launchModule(icon->module);
            }
        } else {
            // Fallback: use position
            wxPoint pos = event.GetPosition();
            DesktopModuleIcon* icon =
                dynamic_cast<DesktopModuleIcon*>(FindIconAt(pos));
            if (icon && icon->module) {
                launchModule(icon->module);
            }
        }
    }

    event.Skip();
}

void DesktopWindow::OnIconMouseMove(wxMouseEvent& event) {
    if (!isDragging_ || !draggingIcon_ || !event.Dragging() || !event.LeftIsDown()) {
        event.Skip();
        return;
    }

    wxWindow* window = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (!window) {
        event.Skip();
        return;
    }

    wxPoint localPos = event.GetPosition();           // coords in 'window'
    wxPoint screenPos = window->ClientToScreen(localPos);
    wxPoint desktopPos = ScreenToClient(screenPos);
    wxPoint delta = desktopPos - dragStartMousePos_;
    wxPoint newPos = dragStartIconPos_ + delta;

    // Optional: clamp to desktop bounds
    wxSize clientSize = GetClientSize();
    if (newPos.x < 0)
        newPos.x = 0;
    if (newPos.y < 0)
        newPos.y = 0;
    if (newPos.x + iconSize_.GetWidth() > clientSize.GetWidth())
        newPos.x = clientSize.GetWidth() - iconSize_.GetWidth();
    if (newPos.y + iconSize_.GetHeight() > clientSize.GetHeight())
        newPos.y = clientSize.GetHeight() - iconSize_.GetHeight();

    draggingIcon_->position = newPos;
    draggingIcon_->widget->SetPosition(newPos);

    Refresh();
    event.Skip();
}

DesktopIcon* DesktopWindow::FindIconAt(const wxPoint& pos) {
    for (auto& icon : icons_) {
        if (icon.widget) {
            wxRect rect = icon.widget->GetRect();
            if (rect.Contains(pos)) {
                return &icon;
            }
        }
    }
    for (auto& icon : volumeIcons_) {
        if (icon.widget) {
            wxRect rect = icon.widget->GetRect();
            if (rect.Contains(pos)) {
                return &icon;
            }
        }
    }
    return nullptr;
}

DesktopModuleIcon* DesktopWindow::FindIconByModule(const std::string& uri) {
    for (auto& icon : icons_) {
        if (icon.module && icon.module->getFullUri() == uri) {
            return &icon;
        }
    }
    return nullptr;
}

DesktopIcon* DesktopWindow::FindIconByWindow(wxWindow* window) {
    if (!window) {
        return nullptr;
    }

    for (auto& icon : icons_) {
        if (!icon.widget)
            continue;
        if (icon.widget == window || window->GetParent() == icon.widget) {
            return &icon;
        }
    }
    for (auto& icon : volumeIcons_) {
        if (!icon.widget)
            continue;
        if (icon.widget == window || window->GetParent() == icon.widget) {
            return &icon;
        }
    }
    return nullptr;
}

} // namespace os

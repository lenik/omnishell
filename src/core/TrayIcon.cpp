#include "TrayIcon.hpp"

#include <wx/icon.h>
#include <wx/log.h>

namespace os {

BEGIN_EVENT_TABLE(TrayIcon, wxTaskBarIcon)
    EVT_TASKBAR_LEFT_DOWN(TrayIcon::OnTrayIconLeftClick)
    EVT_TASKBAR_RIGHT_DOWN(TrayIcon::OnTrayIconRightClick)
    EVT_TASKBAR_LEFT_DCLICK(TrayIcon::OnTrayIconDoubleClick)
END_EVENT_TABLE()

TrayIcon::TrayIcon() 
    : contextMenu_(nullptr)
{
}

TrayIcon::~TrayIcon() {
    RemoveIcon();
    if (contextMenu_) {
        delete contextMenu_;
    }
}

void TrayIcon::setIcon(const wxIcon& icon) {
    if (!SetIcon(icon, "OmniShell Tray Icon")) {
        wxLogError("Failed to set tray icon");
    }
}

void TrayIcon::setTooltip(const std::string& tooltip) {
    // For wxWidgets 3.0, we need to keep the icon and just update tooltip
    // Store icon reference and reuse it
    static wxIcon storedIcon;
    if (storedIcon.IsOk()) {
        SetIcon(storedIcon, tooltip);
    }
    // Note: tooltip update may not work in wx 3.0 without recreating icon
}

void TrayIcon::onClick(ClickHandler handler) {
    onClickHandler_ = handler;
}

void TrayIcon::onRightClick(RightClickHandler handler) {
    onRightClickHandler_ = handler;
}

void TrayIcon::setContextMenu(wxMenu* menu) {
    if (contextMenu_) {
        delete contextMenu_;
    }
    contextMenu_ = menu;
}

void TrayIcon::showNotification(const std::string& title, const std::string& text, int timeout) {
    if (IsIconInstalled()) {
        #if wxCHECK_VERSION(3, 1, 0)
        ShowNotification(title, text, timeout);
        #else
        wxLogInfo("Notification: %s - %s", title, text);
        #endif
    } else {
        wxLogWarning("Cannot show notification: tray icon not installed");
    }
}

void TrayIcon::remove() {
    RemoveIcon();
}

wxMenu* TrayIcon::CreatePopupMenu() {
    if (contextMenu_) {
        // Create a copy of the menu
        wxMenu* copy = new wxMenu();
        // Copy menu items (simplified - just create a new menu)
        for (wxMenuItem* item : contextMenu_->GetMenuItems()) {
            if (item->IsSeparator()) {
                copy->AppendSeparator();
            } else {
                copy->Append(item->GetId(), item->GetItemLabel(), item->GetItemLabelText());
            }
        }
        return copy;
    }
    return nullptr;
}

void TrayIcon::OnTrayIconLeftClick(wxTaskBarIconEvent& event) {
    if (onClickHandler_) {
        onClickHandler_();
    }
    event.Skip();
}

void TrayIcon::OnTrayIconRightClick(wxTaskBarIconEvent& event) {
    if (onRightClickHandler_) {
        onRightClickHandler_();
    }
    event.Skip();
}

void TrayIcon::OnTrayIconDoubleClick(wxTaskBarIconEvent& event) {
    // Double click treated as left click
    if (onClickHandler_) {
        onClickHandler_();
    }
    event.Skip();
}

} // namespace os

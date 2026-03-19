#ifndef OMNISHELL_CORE_TRAY_ICON_HPP
#define OMNISHELL_CORE_TRAY_ICON_HPP

#include <wx/menu.h>
#include <wx/string.h>
#include <wx/taskbar.h>

#include <functional>
#include <string>

namespace os {

/**
 * System Tray Icon API
 * 
 * Allows modules to create icons in the system tray area.
 * Supports:
 * - Icon display
 * - Tooltips
 * - Click handlers
 * - Context menus
 */
class TrayIcon : public wxTaskBarIcon {
public:
    using ClickHandler = std::function<void()>;
    using RightClickHandler = std::function<void()>;
    
    TrayIcon();
    virtual ~TrayIcon();
    
    /**
     * Set the tray icon
     * @param icon Icon to display (should be small, typically 16x16 or 24x24)
     */
    void setIcon(const wxIcon& icon);
    
    /**
     * Set tooltip text
     * @param tooltip Text to show on hover
     */
    void setTooltip(const std::string& tooltip);
    
    /**
     * Set left click handler
     */
    void onClick(ClickHandler handler);
    
    /**
     * Set right click handler
     * Called before context menu is shown
     */
    void onRightClick(RightClickHandler handler);
    
    /**
     * Set context menu
     * @param menu Menu to display on right-click
     */
    void setContextMenu(wxMenu* menu);
    
    /**
     * Show a balloon notification
     * @param title Notification title
     * @param text Notification text
     * @param timeout Timeout in milliseconds
     */
    void showNotification(const std::string& title, const std::string& text, int timeout = 2000);
    
    /**
     * Remove the tray icon
     */
    void remove();

protected:
    // wxTaskBarIcon overrides
    virtual wxMenu* CreatePopupMenu() override;
    
    // Event handlers
    void OnTrayIconLeftClick(wxTaskBarIconEvent& event);
    void OnTrayIconRightClick(wxTaskBarIconEvent& event);
    void OnTrayIconDoubleClick(wxTaskBarIconEvent& event);

private:
    DECLARE_EVENT_TABLE()
    
    ClickHandler m_onClickHandler;
    RightClickHandler m_onRightClickHandler;
    wxMenu* m_contextMenu;
};

} // namespace os

#endif // OMNISHELL_CORE_TRAY_ICON_HPP

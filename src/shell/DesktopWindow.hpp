#ifndef OMNISHELL_SHELL_DESKTOP_WINDOW_HPP
#define OMNISHELL_SHELL_DESKTOP_WINDOW_HPP

#include "../core/Module.hpp"

#include <wx/dnd.h>
#include <wx/wx.h>

#include <vector>

class VolumeManager;
class Volume;

namespace os {

/**
 * Desktop Icon - module application
 */
struct DesktopIcon {
    ModulePtr module;
    wxPoint position;
    wxStaticBitmap* bitmap;
    wxStaticText* label;
    bool selected;

    DesktopIcon()
        : bitmap(nullptr)
        , label(nullptr)
        , selected(false)
    {}
};

/**
 * Volume Icon - VFS volume on desktop
 */
struct VolumeDesktopIcon {
    Volume* volume;
    wxPoint position;
    wxStaticBitmap* bitmap;
    wxStaticText* label;

    VolumeDesktopIcon() : volume(nullptr), bitmap(nullptr), label(nullptr) {}
};

/**
 * Desktop Window
 * 
 * Main desktop area displaying:
 * - Module icons
 * - Background image/color
 * - Context menus
 * 
 * Supports:
 * - Icon layout and arrangement
 * - Drag and drop
 * - Double-click to launch
 * - Right-click context menu
 */
class DesktopWindow : public wxPanel {
public:
    DesktopWindow(wxWindow* parent);
    virtual ~DesktopWindow();
    
    /**
     * Add a module icon to desktop
     */
    void addIcon(ModulePtr module, const wxPoint& position = wxDefaultPosition);
    
    /**
     * Remove a module icon
     */
    void removeIcon(const std::string& moduleUri);
    
    /**
     * Set VolumeManager for volume icons (call before addVolumeIcons).
     */
    void setVolumeManager(VolumeManager* vm);

    /**
     * Add one desktop icon per volume (call after setVolumeManager).
     */
    void addVolumeIcons();

    /**
     * Clear all icons (module + volume)
     */
    void clearIcons();

    /**
     * Arrange icons in grid
     */
    void arrangeIcons();
    
    /**
     * Set desktop background color
     */
    void setBackgroundColor(const wxColour& color);
    
    /**
     * Set desktop background image
     */
    void setBackgroundImage(const wxBitmap& bitmap);
    
    /**
     * Get all desktop icons
     */
    const std::vector<DesktopIcon>& getIcons() const { return icons_; }
    
    /**
     * Launch a module
     */
    void launchModule(ModulePtr module);

protected:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnLeftDoubleClick(wxMouseEvent& event);
    void OnRightClick(wxMouseEvent& event);
    void OnEraseBackground(wxEraseEvent& event);
    
    void OnIconLeftDoubleClick(wxMouseEvent& event);
    void OnIconRightClick(wxMouseEvent& event);
    
    void CreateIconControls(DesktopIcon& icon);
    void CreateVolumeIconControls(VolumeDesktopIcon& icon);
    void UpdateIconPositions();
    DesktopIcon* FindIconAt(const wxPoint& pos);
    DesktopIcon* FindIconByModule(const std::string& uri);

private:
    DECLARE_EVENT_TABLE()

    VolumeManager* volumeManager_;
    std::vector<DesktopIcon> icons_;
    std::vector<VolumeDesktopIcon> volumeIcons_;
    wxColour backgroundColor_;
    wxBitmap backgroundImage_;
    wxSize iconSize_;
    int iconSpacing_;
    int margin_;
};

} // namespace os

#endif // OMNISHELL_SHELL_DESKTOP_WINDOW_HPP

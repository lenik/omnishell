#ifndef OMNISHELL_SHELL_DESKTOP_WINDOW_HPP
#define OMNISHELL_SHELL_DESKTOP_WINDOW_HPP

#include "../core/Module.hpp"
#include "../wx/LabeledIcon.hpp"

#include <wx/dnd.h>
#include <wx/wx.h>

#include <vector>

class VolumeManager;
class Volume;

namespace os {

struct DesktopIcon {
    wxPoint position;
    LabeledIcon* widget{nullptr};
    bool selected{false};

    virtual ~DesktopIcon() = default;

    virtual bool isModule() const { return false; }
    virtual bool isVolume() const { return false; }
};

/**
 * Desktop Icon - module application
 */
struct DesktopModuleIcon : public DesktopIcon {
    ModulePtr module{nullptr};

    bool isModule() const override { return true; }
};

/**
 * Volume Icon - VFS volume on desktop
 */
struct VolumeDesktopIcon : public DesktopIcon {
    Volume* volume{nullptr};

    bool isVolume() const override { return true; }
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

    /** Load background settings from RegistryDb (color or image). */
    void loadBackgroundSettings();

    /**
     * Get all desktop module icons
     */
    const std::vector<DesktopModuleIcon>& getIcons() const { return m_icons; }

    /**
     * Launch a module
     */
    void launchModule(ModulePtr module);

    // Persist and restore icon layout
    void saveLayout() const;
    void loadLayout();

  protected:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnLeftDoubleClick(wxMouseEvent& event);
    void OnRightClick(wxMouseEvent& event);
    void OnEraseBackground(wxEraseEvent& event);

    void OnIconLeftDoubleClick(wxMouseEvent& event);
    void OnIconRightClick(wxMouseEvent& event);
    void OnIconLeftDown(wxMouseEvent& event);
    void OnIconLeftUp(wxMouseEvent& event);
    void OnIconMouseMove(wxMouseEvent& event);

    void CreateIconControls(DesktopIcon& icon);
    void CreateVolumeIconControls(VolumeDesktopIcon& icon);
    void UpdateIconPositions();
    DesktopIcon* FindIconAt(const wxPoint& pos);
    DesktopModuleIcon* FindIconByModule(const std::string& uri);
    DesktopIcon* FindIconByWindow(wxWindow* window);

  private:
    DECLARE_EVENT_TABLE()

    VolumeManager* m_volumeManager;
    std::vector<DesktopModuleIcon> m_icons;
    std::vector<VolumeDesktopIcon> m_volumeIcons;
    wxColour m_backgroundColor;
    wxBitmap m_backgroundImage;
    wxImage m_backgroundImageSrc;

    void UpdateScaledBackground();
    wxSize m_iconSize;
    int m_iconSpacing;
    int m_margin;

    bool m_isDragging = false;
    DesktopIcon* m_draggingIcon = nullptr;
    // Offset from icon's top-left to the mouse position at drag start.
    wxPoint m_dragOffset;
};

} // namespace os

#endif // OMNISHELL_SHELL_DESKTOP_WINDOW_HPP

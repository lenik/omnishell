#ifndef OMNISHELL_SHELL_SHELL_HPP
#define OMNISHELL_SHELL_SHELL_HPP

#include "DesktopWindow.hpp"
#include "StartMenu.hpp"
#include "Taskbar.hpp"

#include "../core/ModuleRegistry.hpp"
#include "../core/ServiceManager.hpp"

#include <wx/wx.h>

class VolumeManager;

namespace os {

/**
 * OmniShell Shell Application
 *
 * Main application class that:
 * - Creates the main window
 * - Initializes module system and VFS (VolumeManager)
 * - Manages lifecycle
 * - Coordinates UI components
 */
class ShellApp : public wxApp {
public:
    ShellApp();
    virtual ~ShellApp();

    virtual bool OnInit() override;
    virtual int OnExit() override;

    /**
     * Get shell instance
     */
    static ShellApp* getInstance();

    /**
     * Get volume manager (VFS). Never null after OnInit.
     */
    VolumeManager* getVolumeManager() const { return volumeManager_; }

    /**
     * Get desktop window
     */
    DesktopWindow* getDesktopWindow() const { return desktop_; }

    /**
     * Get taskbar
     */
    Taskbar* getTaskbar() const { return taskbar_; }

    /**
     * Get start menu
     */
    StartMenu* getStartMenu() const { return startMenu_; }

    /**
     * Launch a module
     */
    void launchModule(ModulePtr module);

    /**
     * Refresh desktop icons (volumes + modules)
     */
    void refreshDesktop();

private:
    bool initializeModules();
    void createUI();
    void setupEventHandlers();

    VolumeManager* volumeManager_;
    wxFrame* mainWindow_;
    DesktopWindow* desktop_;
    Taskbar* taskbar_;
    StartMenu* startMenu_;

    static ShellApp* instance_;
};

// Application macro
wxDECLARE_APP(ShellApp);

} // namespace os

#endif // OMNISHELL_SHELL_SHELL_HPP

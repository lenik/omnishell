#ifndef OMNISHELL_SHELL_SHELL_HPP
#define OMNISHELL_SHELL_SHELL_HPP

#include "DesktopWindow.hpp"
#include "StartMenu.hpp"
#include "Taskbar.hpp"

#include "../core/ModuleRegistry.hpp"
#include "../wx/wxcWindow.hpp"
#include "VfsDaemonHost.hpp"

#include <bas/wx/app.hpp>

#include <wx/wx.h>

#include <string>
#include <vector>

// #include <bas/wx/uiframe.hpp>

class VolumeManager;

namespace os {

class ShellApp : public uiApp {
  public:
    explicit ShellApp(std::string name = "OmniShell");
    virtual ~ShellApp();

    virtual bool OnUserInit() override;
    virtual int OnExit() override;

    static ShellApp* getInstance();

    const std::string& getName() const { return m_name; }

    VolumeManager* getVolumeManager() const { return m_volumeManager; }
    ModuleRegistry* getModuleRegistry() const { return m_moduleRegistry; }
    DesktopWindow* getDesktopWindow() const { return m_desktop; }
    Taskbar* getTaskbar() const { return m_taskbar; }
    StartMenu* getStartMenu() const { return m_startMenu; }
    /** Main shell frame (desktop host). Excluded from Task Manager "end task" on that window. */
    wxcFrame* getMainWindow() const { return m_mainWindow; }

    VfsDaemonHost& vfsDaemon() { return m_vfsDaemon; }
    const VfsDaemonHost& vfsDaemon() const { return m_vfsDaemon; }

    void openExplorerAt(const std::string& dir);

    void toggleStartMenu();

    void launchModule(ModulePtr module);
    /** Same as launchModule(module) but with association-style args (e.g. vol://…/path). */
    void launchModule(ModulePtr module, std::vector<std::string> args);
    void refreshDesktop();

  private:
    bool initializeModules();
    void createUI();
    void setupEventHandlers();
    void positionStartMenu();
    void processStartFiles();

    std::string m_name;
    VfsDaemonHost m_vfsDaemon;
    VolumeManager* m_volumeManager;
    ModuleRegistry* m_moduleRegistry;
    wxcFrame* m_mainWindow;
    DesktopWindow* m_desktop;
    Taskbar* m_taskbar;
    StartMenu* m_startMenu;

    static ShellApp* m_instance;
};

} // namespace os

#endif // OMNISHELL_SHELL_SHELL_HPP

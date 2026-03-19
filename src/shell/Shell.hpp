#ifndef OMNISHELL_SHELL_SHELL_HPP
#define OMNISHELL_SHELL_SHELL_HPP

#include "DesktopWindow.hpp"
#include "StartMenu.hpp"
#include "Taskbar.hpp"

#include "../core/ModuleRegistry.hpp"

#include <bas/wx/app.hpp>

#include <wx/wx.h>

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

    void openExplorerAt(const std::string& dir);

    void toggleStartMenu();

    void launchModule(ModulePtr module);
    void refreshDesktop();

  private:
    bool initializeModules();
    void createUI();
    void setupEventHandlers();
    void positionStartMenu();

    std::string m_name;
    VolumeManager* m_volumeManager;
    ModuleRegistry* m_moduleRegistry;
    wxFrame* m_mainWindow;
    DesktopWindow* m_desktop;
    Taskbar* m_taskbar;
    StartMenu* m_startMenu;

    static ShellApp* m_instance;
};

} // namespace os

#endif // OMNISHELL_SHELL_SHELL_HPP

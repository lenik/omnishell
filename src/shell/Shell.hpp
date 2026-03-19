#ifndef OMNISHELL_SHELL_SHELL_HPP
#define OMNISHELL_SHELL_SHELL_HPP

#include "DesktopWindow.hpp"
#include "StartMenu.hpp"
#include "Taskbar.hpp"

#include "../core/ModuleRegistry.hpp"

#include <wx/wx.h>

#include <bas/wx/app.hpp>
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

    const std::string& getName() const { return name_; }

    VolumeManager* getVolumeManager() const { return volumeManager_; }
    ModuleRegistry* getModuleRegistry() const { return moduleRegistry_; }
    DesktopWindow* getDesktopWindow() const { return desktop_; }
    Taskbar* getTaskbar() const { return taskbar_; }
    StartMenu* getStartMenu() const { return startMenu_; }

    void openExplorerAt(const std::string& dir);

    void toggleStartMenu();

    void launchModule(ModulePtr module);
    void refreshDesktop();

  private:
    bool initializeModules();
    void createUI();
    void setupEventHandlers();
    void positionStartMenu();

    std::string name_;
    VolumeManager* volumeManager_;
    ModuleRegistry* moduleRegistry_;
    wxFrame* mainWindow_;
    DesktopWindow* desktop_;
    Taskbar* taskbar_;
    StartMenu* startMenu_;

    static ShellApp* instance_;
};

} // namespace os

#endif // OMNISHELL_SHELL_SHELL_HPP

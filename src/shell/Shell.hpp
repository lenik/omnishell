#ifndef OMNISHELL_SHELL_SHELL_HPP
#define OMNISHELL_SHELL_SHELL_HPP

#include "DesktopWindow.hpp"
#include "StartMenu.hpp"
#include "Taskbar.hpp"

#include "../core/ModuleRegistry.hpp"
#include "../core/ServiceManager.hpp"

#include <wx/wx.h>

#include <bas/wx/app.hpp>
#include <bas/wx/appframe.hpp>

class VolumeManager;

namespace os {

class ShellApp : public uiApp {
  public:
    ShellApp();
    virtual ~ShellApp();

    virtual bool OnUserInit() override;
    virtual int OnExit() override;

    static ShellApp* getInstance();

    VolumeManager* getVolumeManager() const { return volumeManager_; }
    DesktopWindow* getDesktopWindow() const { return desktop_; }
    Taskbar* getTaskbar() const { return taskbar_; }
    StartMenu* getStartMenu() const { return startMenu_; }

    void launchModule(ModulePtr module);
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

} // namespace os

#endif // OMNISHELL_SHELL_SHELL_HPP

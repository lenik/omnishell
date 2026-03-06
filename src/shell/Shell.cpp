#include "Shell.hpp"

#include "../core/App.hpp"

#include <wx/log.h>
#include <wx/splitter.h>

namespace os {

wxIMPLEMENT_APP_NO_MAIN(ShellApp);

ShellApp* ShellApp::instance_ = nullptr;

ShellApp::ShellApp()
    : volumeManager_(nullptr)
    , mainWindow_(nullptr)
    , desktop_(nullptr)
    , taskbar_(nullptr)
    , startMenu_(nullptr)
{
    instance_ = this;
}

ShellApp::~ShellApp() {
    ServiceManager::getInstance().stopAllServices();
    ModuleRegistry::getInstance().uninstallAll();
    instance_ = nullptr;
}

bool ShellApp::OnInit() {
    if (!wxApp::OnInit()) {
        return false;
    }

    volumeManager_ = app.volumeManager.get();
    if (!volumeManager_) {
        wxLogError("VolumeManager not initialized");
        return false;
    }

    SetAppName("OmniShell");
    SetAppDisplayName("OmniShell Desktop Environment");
    SetVendorName("OmniShell Project");

    if (!initializeModules()) {
        wxLogError("Failed to initialize modules");
        return false;
    }

    createUI();
    setupEventHandlers();
    ServiceManager::getInstance().startAllServices();
    refreshDesktop();

    return true;
}

int ShellApp::OnExit() {
    wxLogInfo("Shell exiting");
    return wxApp::OnExit();
}

ShellApp* ShellApp::getInstance() {
    return instance_;
}

void ShellApp::launchModule(ModulePtr module) {
    if (!module) {
        wxLogError("Cannot launch null module");
        return;
    }
    
    if (!module->isEnabled()) {
        wxLogWarning("Module %s is not enabled", module->getFullUri());
        wxMessageBox("Module is not enabled", "Error", wxOK | wxICON_WARNING);
        return;
    }
    
    try {
        wxLogInfo("Launching module: %s", module->label);
        module->recordExecution();
        module->run();
    } catch (const std::exception& e) {
        wxLogError("Failed to launch module %s: %s", 
                   module->getFullUri(), e.what());
        wxMessageBox("Failed to launch " + module->label + ": " + e.what(),
                     "Error", wxOK | wxICON_ERROR);
    }
}

void ShellApp::refreshDesktop() {
    if (desktop_) {
        desktop_->clearIcons();
        desktop_->addVolumeIcons();
        auto& registry = ModuleRegistry::getInstance();
        for (auto& module : registry.getVisibleModules()) {
            desktop_->addIcon(module);
        }
        desktop_->arrangeIcons();
    }
}

bool ShellApp::initializeModules() {
    wxLogInfo("Initializing module system");
    
    // Install all registered modules
    ModuleRegistry::getInstance().installAll();
    
    return true;
}

void ShellApp::createUI() {
    wxLogInfo("Creating shell UI");
    
    // Create main frame
    mainWindow_ = new wxFrame(nullptr, wxID_ANY, "OmniShell",
                               wxDefaultPosition, wxSize(1024, 768));
    mainWindow_->SetMinSize(wxSize(800, 600));
    mainWindow_->SetBackgroundColour(*wxWHITE);
    
    // Create vertical sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create desktop window and set VFS for volume icons
    desktop_ = new DesktopWindow(mainWindow_);
    desktop_->setVolumeManager(volumeManager_);
    mainSizer->Add(desktop_, 1, wxEXPAND);
    
    // Create taskbar
    taskbar_ = new Taskbar(mainWindow_);
    mainSizer->Add(taskbar_, 0, wxEXPAND);
    
    mainWindow_->SetSizer(mainSizer);
    
    // Create start menu
    startMenu_ = new StartMenu(mainWindow_);
    
    // Set start menu in taskbar
    // taskbar_->setStartMenu(startMenu_->GetMenu()); // TODO: expose menu from start menu
    
    // Show main window
    mainWindow_->Show(true);
    mainWindow_->Maximize();
    
    wxLogInfo("Shell UI created");
}

void ShellApp::setupEventHandlers() {
    // Setup start menu launch callback
    if (startMenu_) {
        startMenu_->setLaunchCallback([this](ModulePtr module) {
            launchModule(module);
        });
    }
}

} // namespace os

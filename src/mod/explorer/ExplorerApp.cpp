#include "ExplorerApp.hpp"

#include "ExplorerFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../shell/Shell.hpp"

#include <bas/volume/VolumeManager.hpp>

#include <wx/log.h>
#include <wx/msgdlg.h>

#include <memory>

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

namespace os {

namespace {
constexpr const char* kExplorerModuleUri = "omnishell.Explorer";
}

OMNISHELL_REGISTER_MODULE(kExplorerModuleUri, ExplorerApp)

ExplorerApp::ExplorerApp(CreateModuleContext* ctx) //
    : Module(ctx) {
    initializeMetadata();
}

ExplorerApp::~ExplorerApp() = default;

void ExplorerApp::initializeMetadata() {
    uri = kExplorerModuleUri;
    name = "explorer";
    label = "Explorer";
    description = "Browse files and folders";
    doc = "A simple file explorer for browsing volumes and directories.";
    categoryId = ID_CATEGORY_SYSTEM;

    image = os::app.getIconTheme()->icon("explorer", "icon");
}

ProcessPtr ExplorerApp::run(const RunConfig& config) {
    (void)config;
    VolumeManager* vm =
        ShellApp::getInstance() ? ShellApp::getInstance()->getVolumeManager() : nullptr;
    if (!vm) {
        wxMessageBox("No volume manager", "Explorer", wxOK | wxICON_ERROR);
        return nullptr;
    }
    Volume* vol = vm->getDefaultVolume();
    if (!vol) {
        wxMessageBox("No default volume", "Explorer", wxOK | wxICON_ERROR);
        return nullptr;
    }
    return openInternal(vol, "/");
}

ProcessPtr ExplorerApp::open(Volume* volume, const std::string& dir) {
    if (!volume) {
        VolumeManager* vm =
            ShellApp::getInstance() ? ShellApp::getInstance()->getVolumeManager() : nullptr;
        if (!vm) {
            wxMessageBox("No volume manager", "Explorer", wxOK | wxICON_ERROR);
            return nullptr;
        }
        volume = vm->getDefaultVolume();
        if (!volume) {
            wxMessageBox("No default volume", "Explorer", wxOK | wxICON_ERROR);
            return nullptr;
        }
    }
    return openInternal(volume, dir);
}

ProcessPtr ExplorerApp::openInternal(Volume* volume, const std::string& dir) {
    if (!volume)
        return nullptr;

    VolumeManager* vm =
        ShellApp::getInstance() ? ShellApp::getInstance()->getVolumeManager() : nullptr;

    auto proc = std::make_shared<Process>();
    proc->uri = kExplorerModuleUri;
    proc->name = "explorer";
    proc->label = "Explorer";
    proc->icon = os::app.getIconTheme()->icon("explorer", "icon");

    auto* frame = new ExplorerFrame(&app, vm, volume, dir);
    frame->SetSize(wxSize(800, 600));
    frame->Centre();
    frame->Show(true);

    proc->addWindow(frame);
    return proc;
}

} // namespace os

#include "ExplorerApp.hpp"

#include "../../core/ModuleRegistry.hpp"
#include "../../shell/Shell.hpp"

#include <bas/volume/VolumeManager.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/log.h>
#include <wx/msgdlg.h>

#include <memory>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.explorer", ExplorerApp)

ExplorerApp::ExplorerApp(CreateModuleContext* ctx)
    : Module(ctx) {
    initializeMetadata();
}

ExplorerApp::~ExplorerApp() = default;

void ExplorerApp::initializeMetadata() {
    uri = "omnishell";
    name = "explorer";
    label = "Explorer";
    description = "Browse files and folders";
    doc = "A simple file explorer for browsing volumes and directories.";
    categoryId = ID_CATEGORY_SYSTEM;

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "folder-add.svg"));
}

ProcessPtr ExplorerApp::run() {
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
    proc->uri = "omnishell";
    proc->name = "explorer";
    proc->label = "Explorer";
    std::string iconDir = "streamline-vectors/core/pop/interface-essential";
    proc->icon = ImageSet(Path(iconDir, "folder-add.svg"));

    // Own the core for the lifetime of the window (wx Bind copies functors, so use shared_ptr).
    auto core = std::make_shared<ExplorerCore>(vm, volume, dir);
    uiFrame* frame = new uiFrame("Explorer");
    frame->addFragment(core.get());
    frame->createView();
    frame->SetSize(wxSize(800, 600));
    frame->Centre();
    frame->Show(true);

    frame->Bind(wxEVT_CLOSE_WINDOW, [core](wxCloseEvent& e) {
        (void)core; // keep alive until frame destroyed
        e.Skip();
    });

    proc->addWindow(frame);
    return proc;
}

} // namespace os


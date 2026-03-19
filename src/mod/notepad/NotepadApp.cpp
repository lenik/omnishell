#include "NotepadApp.hpp"

#include "../../core/ModuleRegistry.hpp"

#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#include <string>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.notepad", NotepadApp)

NotepadApp::NotepadApp(CreateModuleContext* ctx)
    : Module(ctx)
    , core_(ctx->getVolumeManager()) //
{
    initializeMetadata();
}

NotepadApp::~NotepadApp() {
}

void NotepadApp::initializeMetadata() {
    uri = "omnishell";
    name = "notepad";
    label = "Notepad";
    description = "Simple text editor";
    doc = "A basic text editor for viewing and editing text files.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "blank-notepad.svg"));
}

ProcessPtr NotepadApp::run() {
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    uiFrame* frame = new uiFrame("Notepad");
    frame->addFragment(&core_);
    frame->createView();
    // createMenuBar();
    // editor_ = new wxTextCtrl(frame_, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
    //                          wxTE_MULTILINE | wxTE_RICH2 | wxTE_PROCESS_ENTER);
    // wxFont font(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    // editor_->SetFont(font);
    // statusBar_ = frame_->CreateStatusBar(2);
    // statusBar_->SetStatusText("Line 1, Column 1", 0);
    // statusBar_->SetStatusText("UTF-8", 1);
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

ProcessPtr NotepadApp::open(VolumeManager* volumeManager, VolumeFile file) {
    auto proc = std::make_shared<Process>();
    proc->uri = "omnishell";
    proc->name = "notepad";
    proc->label = "Notepad";
    std::string dir = "streamline-vectors/core/pop/interface-essential";
    proc->icon = ImageSet(Path(dir, "blank-notepad.svg"));

    auto core = std::make_shared<NotepadCore>(volumeManager);
    uiFrame* frame = new uiFrame("Notepad");
    frame->addFragment(core.get());
    frame->createView();
    frame->Centre();
    frame->Show(true);
    core->openFile(file);

    frame->Bind(wxEVT_CLOSE_WINDOW, [core](wxCloseEvent& e) {
        (void)core;
        e.Skip();
    });

    proc->addWindow(frame);
    return proc;
}

void NotepadApp::onAbout(PerformContext* ctx) {
    wxMessageBox("Notepad for OmniShell\n\nA simple text editor (VFS)", "About Notepad",
                 wxOK | wxICON_INFORMATION);
}

} // namespace os

#include "NotepadApp.hpp"

#include "../../core/ModuleRegistry.hpp"
#include "../../shell/Shell.hpp"
#include "../../ui/dialogs/ChooseFileDialog.hpp"

#include <bas/volume/VolumeFile.hpp>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#include <string>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.notepad", NotepadApp)

static wxString displayNameFromPath(const std::string& path) {
    if (path.empty())
        return "Untitled";
    size_t last = path.rfind('/');
    if (last == std::string::npos)
        return wxString(path);
    if (last + 1 >= path.size())
        return "Untitled";
    return wxString(path.substr(last + 1));
}

NotepadApp::NotepadApp()
    : frame_(nullptr), editor_(nullptr), statusBar_(nullptr), modified_(false) {
    initializeMetadata();
}

NotepadApp::~NotepadApp() {
    if (frame_) {
        frame_->Destroy();
    }
}

void NotepadApp::initializeMetadata() {
    uri = "omnishell";
    name = "notepad";
    label = "Notepad";
    description = "Simple text editor";
    doc = "A basic text editor for viewing and editing text files.";

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "blank-notepad.svg"))
                .scale(16, 16, Path(dir, "blank-notepad-16x16.png"))
                .scale(24, 24, Path(dir, "blank-notepad-24x24.png"))
                .scale(32, 32, Path(dir, "blank-notepad-32x32.png"))
                .scale(48, 48, Path(dir, "blank-notepad-48x48.png"));
}

void NotepadApp::run() {
    if (frame_) {
        frame_->Raise();
        frame_->SetFocus();
        return;
    }
    createMainWindow();
    setupEvents();
    frame_->Show(true);
}

void NotepadApp::createMainWindow() {
    frame_ = new wxFrame(nullptr, wxID_ANY, "Notepad", wxDefaultPosition, wxSize(800, 600));
    createMenuBar();
    editor_ = new wxTextCtrl(frame_, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE | wxTE_RICH2 | wxTE_PROCESS_ENTER);
    wxFont font(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    editor_->SetFont(font);
    statusBar_ = frame_->CreateStatusBar(2);
    statusBar_->SetStatusText("Line 1, Column 1", 0);
    statusBar_->SetStatusText("UTF-8", 1);
    frame_->Centre();
}

void NotepadApp::createMenuBar() {
    wxMenuBar* menuBar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(wxID_NEW, "&New\tCtrl+N");
    fileMenu->Append(wxID_OPEN, "&Open...\tCtrl+O");
    fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S");
    fileMenu->Append(wxID_SAVEAS, "Save &As...\tCtrl+Shift+S");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4");
    menuBar->Append(fileMenu, "&File");
    wxMenu* editMenu = new wxMenu();
    editMenu->Append(wxID_UNDO, "&Undo\tCtrl+Z");
    editMenu->Append(wxID_REDO, "&Redo\tCtrl+Y");
    editMenu->AppendSeparator();
    editMenu->Append(wxID_CUT, "Cu&t\tCtrl+X");
    editMenu->Append(wxID_COPY, "&Copy\tCtrl+C");
    editMenu->Append(wxID_PASTE, "&Paste\tCtrl+V");
    editMenu->AppendSeparator();
    editMenu->Append(wxID_FIND, "&Find...\tCtrl+F");
    editMenu->Append(wxID_REPLACE, "&Replace...\tCtrl+H");
    menuBar->Append(editMenu, "&Edit");
    wxMenu* viewMenu = new wxMenu();
    viewMenu->AppendCheckItem(wxID_ANY, "Show &Line Numbers");
    viewMenu->Check(viewMenu->FindItemByPosition(0)->GetId(), true);
    menuBar->Append(viewMenu, "&View");
    wxMenu* encodingMenu = new wxMenu();
    encodingMenu->Append(wxID_ANY, "UTF-8");
    encodingMenu->Append(wxID_ANY, "UTF-16 LE");
    encodingMenu->Append(wxID_ANY, "UTF-16 BE");
    menuBar->Append(encodingMenu, "E&ncoding");
    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, "&About");
    menuBar->Append(helpMenu, "&Help");
    frame_->SetMenuBar(menuBar);
}

void NotepadApp::setupEvents() {
    frame_->Bind(wxEVT_MENU, &NotepadApp::OnNew, this, wxID_NEW);
    frame_->Bind(wxEVT_MENU, &NotepadApp::OnOpen, this, wxID_OPEN);
    frame_->Bind(wxEVT_MENU, &NotepadApp::OnSave, this, wxID_SAVE);
    frame_->Bind(wxEVT_MENU, &NotepadApp::OnSaveAs, this, wxID_SAVEAS);
    frame_->Bind(wxEVT_MENU, &NotepadApp::OnExit, this, wxID_EXIT);
    frame_->Bind(wxEVT_MENU, &NotepadApp::OnAbout, this, wxID_ABOUT);
    frame_->Bind(wxEVT_MENU, &NotepadApp::OnFind, this, wxID_FIND);
    frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        if (modified_) {
            int ret =
                wxMessageBox("Save changes?", "Notepad", wxYES_NO | wxCANCEL | wxICON_QUESTION);
            if (ret == wxYES) {
                if (currentFile_.isNotEmpty()) {
                    saveFile(currentFile_);
                } else {
                    if (!saveFileAs())
                        return;
                }
            } else if (ret == wxCANCEL) {
                return;
            }
        }
        frame_->Destroy();
        frame_ = nullptr;
    });
    editor_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        if (!modified_) {
            modified_ = true;
            wxString title = "Notepad - ";
            if (currentFile_.isNotEmpty()) {
                title += displayNameFromPath(currentFile_.getPath()) + " *";
            } else {
                title += "Untitled *";
            }
            frame_->SetTitle(title);
        }
    });
}

bool NotepadApp::loadFile(const VolumeFile& vf) {
    if (vf.isEmpty())
        return false;
    try {
        std::string content = vf.readFileString("UTF-8");
        editor_->SetValue(wxString(content));
        modified_ = false;
        currentFile_ = vf;
        frame_->SetTitle("Notepad - " + displayNameFromPath(vf.getPath()));
        statusBar_->SetStatusText("UTF-8", 1);
        return true;
    } catch (const std::exception& e) {
        wxMessageBox(wxString("Cannot open file: ") + e.what(), "Error", wxOK | wxICON_ERROR);
        return false;
    }
}

bool NotepadApp::saveFile(const VolumeFile& vf) {
    if (vf.isEmpty())
        return false;
    try {
        std::string data = editor_->GetValue().ToStdString();
        vf.writeFileString(data, "UTF-8");
        modified_ = false;
        currentFile_ = vf;
        frame_->SetTitle("Notepad - " + displayNameFromPath(vf.getPath()));
        return true;
    } catch (const std::exception& e) {
        wxMessageBox(wxString("Cannot save file: ") + e.what(), "Error", wxOK | wxICON_ERROR);
        return false;
    }
}

bool NotepadApp::saveFileAs() {
    VolumeManager* vm =
        ShellApp::getInstance() ? ShellApp::getInstance()->getVolumeManager() : nullptr;
    if (!vm) {
        wxMessageBox("No volume manager", "Error", wxOK | wxICON_ERROR);
        return false;
    }
    ChooseFileDialog dlg(frame_, vm, "Save As", FileDialogMode::Save, "/", "");
    dlg.addFilter("Text files", "*.txt");
    dlg.addFilter("All files", "*.*");
    dlg.setFileMustExist(false);
    if (dlg.ShowModal() != wxID_OK)
        return false;
    VolumeFile vf = dlg.getVolumeFile();
    if (vf.isEmpty())
        return false;
    return saveFile(vf);
}

void NotepadApp::OnNew(wxCommandEvent& event) {
    if (modified_) {
        int ret = wxMessageBox("Save changes?", "Notepad", wxYES_NO | wxCANCEL | wxICON_QUESTION);
        if (ret == wxYES) {
            if (currentFile_.isNotEmpty()) {
                saveFile(currentFile_);
            } else {
                if (!saveFileAs())
                    return;
            }
        } else if (ret == wxCANCEL) {
            return;
        }
    }
    editor_->Clear();
    modified_ = false;
    currentFile_ = VolumeFile();
    frame_->SetTitle("Notepad - Untitled");
}

void NotepadApp::OnOpen(wxCommandEvent& event) {
    if (modified_) {
        int ret = wxMessageBox("Save changes?", "Notepad", wxYES_NO | wxCANCEL | wxICON_QUESTION);
        if (ret == wxYES) {
            if (currentFile_.isNotEmpty())
                saveFile(currentFile_);
            else if (!saveFileAs())
                return;
        } else if (ret == wxCANCEL)
            return;
    }
    VolumeManager* vm =
        ShellApp::getInstance() ? ShellApp::getInstance()->getVolumeManager() : nullptr;
    if (!vm) {
        wxMessageBox("No volume manager", "Error", wxOK | wxICON_ERROR);
        return;
    }
    ChooseFileDialog dlg(frame_, vm, "Open File", FileDialogMode::Open, "/", "");
    dlg.addFilter("Text files", "*.txt");
    dlg.addFilter("All files", "*.*");
    dlg.setFileMustExist(true);
    if (dlg.ShowModal() == wxID_OK) {
        loadFile(dlg.getVolumeFile());
    }
}

void NotepadApp::OnSave(wxCommandEvent& event) {
    if (currentFile_.isEmpty()) {
        saveFileAs();
    } else {
        saveFile(currentFile_);
    }
}

void NotepadApp::OnSaveAs(wxCommandEvent& event) { saveFileAs(); }

void NotepadApp::OnExit(wxCommandEvent& event) { frame_->Close(); }

void NotepadApp::OnAbout(wxCommandEvent& event) {
    wxMessageBox("Notepad for OmniShell\n\nA simple text editor (VFS)", "About Notepad",
                 wxOK | wxICON_INFORMATION);
}

void NotepadApp::OnFind(wxCommandEvent& event) {
    wxMessageBox("Find dialog not yet implemented", "Notepad", wxOK | wxICON_INFORMATION);
}

void NotepadApp::OnEncoding(wxCommandEvent& event) {
    wxMessageBox("Encoding change not yet implemented", "Notepad", wxOK | wxICON_INFORMATION);
}

} // namespace os

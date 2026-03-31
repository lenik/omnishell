#include "NotepadBody.hpp"

#include "../../ui/dialogs/ChooseFileDialog.hpp"

#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/event.h>
#include <wx/msgdlg.h>
#include <wx/string.h>
#include <wx/textctrl.h>

namespace os {

enum {
    ID_ZOOM_IN = uiFrame::ID_APP_HIGHEST + 1,
    ID_ZOOM_OUT,
    ID_ZOOM_RESET,
};

NotepadBody::NotepadBody(App* app)
    : m_app(app) //
{
    const IconTheme* theme = (m_app ? m_app->getIconTheme() : os::app.getIconTheme());

    int seq = 0;
    action(wxID_NEW, "file", "new", seq++, "&New", "New document")
        .icon(theme->icon("notepad", "file.new"))
        .performFn([this](PerformContext* ctx) { onNew(ctx); })
        .install();
    action(wxID_OPEN, "file", "open", seq++, "&Open...", "Open file")
        .icon(theme->icon("notepad", "file.open"))
        .performFn([this](PerformContext* ctx) { onOpen(ctx); })
        .install();
    action(wxID_SAVE, "file", "save", seq++, "&Save", "Save file")
        .icon(theme->icon("notepad", "file.save"))
        .performFn([this](PerformContext* ctx) { onSave(ctx); })
        .install();
    action(wxID_SAVEAS, "file", "saveas", seq++, "Save &As...", "Save as")
        .icon(theme->icon("notepad", "file.saveas"))
        .performFn([this](PerformContext* ctx) { onSaveAs(ctx); })
        .install();

    seq = 0;
    action(wxID_UNDO, "edit", "undo", seq++, "Undo", "Undo")
        .icon(theme->icon("notepad", "edit.undo"))
        .performFn([this](PerformContext* ctx) { onUndo(ctx); })
        .install();
    action(wxID_REDO, "edit", "redo", seq++, "Redo", "Redo")
        .icon(theme->icon("notepad", "edit.redo"))
        .performFn([this](PerformContext* ctx) { onRedo(ctx); })
        .install();

    seq = 1000;
    action(wxID_SELECTALL, "edit", "select_all", seq++, "Select &All", "Select all")
        .icon(theme->icon("notepad", "edit.select_all"))
        .performFn([this](PerformContext* ctx) { onSelectAll(ctx); })
        .install();
    action(wxID_CLEAR, "edit", "clear", seq++, "Clear", "Clear")
        .icon(theme->icon("notepad", "edit.clear"))
        .performFn([this](PerformContext* ctx) { onClear(ctx); })
        .install();

    action(wxID_CUT, "edit", "cut", seq++, "Cu&t", "Cut")
        .icon(theme->icon("notepad", "edit.cut"))
        .performFn([this](PerformContext* ctx) { onCut(ctx); })
        .install();
    action(wxID_COPY, "edit", "copy", seq++, "&Copy", "Copy")
        .icon(theme->icon("notepad", "edit.copy"))
        .performFn([this](PerformContext* ctx) { onCopy(ctx); })
        .install();
    action(wxID_PASTE, "edit", "paste", seq++, "&Paste", "Paste")
        .icon(theme->icon("notepad", "edit.paste"))
        .performFn([this](PerformContext* ctx) { onPaste(ctx); })
        .install();

    seq = 2000;
    action(ID_ZOOM_IN, "view", "zoom_in", seq++, "Zoom &In", "Zoom in")
        .icon(theme->icon("notepad", "view.zoom_in"))
        .performFn([this](PerformContext* ctx) { onZoomIn(ctx); })
        .install();
    action(ID_ZOOM_OUT, "view", "zoom_out", seq++, "Zoom &Out", "Zoom out")
        .icon(theme->icon("notepad", "view.zoom_out"))
        .performFn([this](PerformContext* ctx) { onZoomOut(ctx); })
        .install();
    action(ID_ZOOM_RESET, "view", "zoom_reset", seq++, "Zoom &Reset", "Zoom reset")
        .icon(theme->icon("notepad", "view.zoom_reset"))
        .performFn([this](PerformContext* ctx) { onZoomReset(ctx); })
        .install();
}

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

void NotepadBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame) {
        wxMessageBox("Parent is not a uiFrame", "Error", wxOK | wxICON_ERROR);
        return;
    }
    m_frame = frame;

    const wxPoint& pos = ctx->getPos();
    const wxSize& size = ctx->getSize();
    m_text = new wxTextCtrl(parent, wxID_ANY, "", pos, size, wxTE_MULTILINE | wxTE_WORDWRAP);

    m_frame->Bind(wxEVT_CLOSE_WINDOW, &NotepadBody::onFrameClose, this);
    m_text->Bind(wxEVT_TEXT, &NotepadBody::onFrameText, this);
}

wxEvtHandler* NotepadBody::getEventHandler() {
    return m_frame ? m_frame->GetEventHandler() : nullptr;
}

void NotepadBody::onFrameClose(wxCloseEvent& event) {
    if (m_modified) {
        int ret = wxMessageBox("Save changes?", "Notepad", wxYES_NO | wxCANCEL | wxICON_QUESTION);
        if (ret == wxYES) {
            if (m_file) {
                uiPersistObject(*m_file);
            } else {
                if (!uiSaveAs())
                    return;
            }
        } else if (ret == wxCANCEL) {
            return;
        }
    }
    m_frame->Destroy();
    m_frame = nullptr;
}

void NotepadBody::onFrameText(wxCommandEvent& event) {
    if (!m_modified) {
        m_modified = true;
        wxString title = "Notepad - ";
        if (m_file) {
            title += displayNameFromPath(m_file->getPath()) + " *";
        } else {
            title += "Untitled *";
        }
        m_frame->SetTitle(title);
    }
}

void NotepadBody::persistObject(VolumeFile file) {
    std::string utf8(m_text->GetValue().ToUTF8());
    file.writeFileString(utf8, m_encoding.ToStdString());
    m_modified = false;
}

bool NotepadBody::uiPersistObject(VolumeFile file) {
    try {
        persistObject(file);
    } catch (const std::exception& e) {
        wxMessageBox(wxString("Cannot save file: ") + e.what(), "Error", wxOK | wxICON_ERROR);
        return false;
    }
    return true;
}

bool NotepadBody::uiSaveAs() {
    if (!volumeManager()) {
        wxMessageBox("No volume manager", "Error", wxOK | wxICON_ERROR);
        return false;
    }
    ChooseFileDialog dlg(m_frame, volumeManager(), "Save As", FileDialogMode::Save, "/", "");
    dlg.addFilter("Text files", "*.txt");
    dlg.addFilter("All files", "*.*");
    dlg.setFileMustExist(false);
    if (dlg.ShowModal() != wxID_OK)
        return false;
    auto vf = dlg.getVolumeFile();
    if (!vf)
        return false;
    if (!uiPersistObject(*vf))
        return false;
    m_file = *vf;
    return true;
}

void NotepadBody::restoreObject(VolumeFile file) {
    std::string utf8 = file.readFileString(m_encoding.ToStdString());
    m_text->SetValue(wxString(utf8));
    m_modified = false;
    m_file = std::move(file);
}

bool NotepadBody::uiRestoreObject(VolumeFile file) {
    if (m_modified) {
        int ret = wxMessageBox("Save changes?", "Notepad", wxYES_NO | wxCANCEL | wxICON_QUESTION);
        if (ret == wxYES) {
            if (m_file) {
                uiPersistObject(*m_file);
            } else {
                if (!uiSaveAs())
                    return false;
            }
        } else if (ret == wxCANCEL) {
            return false;
        }
    }
    try {
        restoreObject(file);
    } catch (const std::exception& e) {
        wxMessageBox(wxString("Cannot restore file: ") + e.what(), "Error", wxOK | wxICON_ERROR);
        return false;
    }
    m_frame->SetTitle("Notepad - " + displayNameFromPath(file.getPath()));
    // statusBar_->SetStatusText("UTF-8", 1);
    return true;
}

void NotepadBody::onNew(PerformContext* ctx) {
    if (m_modified) {
        int ret = wxMessageBox("Save changes?", "Notepad", wxYES_NO | wxCANCEL | wxICON_QUESTION);
        if (ret == wxYES) {
            if (m_file) {
                uiPersistObject(*m_file);
            } else {
                if (!uiSaveAs())
                    return;
            }
        } else if (ret == wxCANCEL) {
            return;
        }
    }
    m_text->Clear();
    m_modified = false;
    m_file.reset();
    m_frame->SetTitle("Notepad - Untitled");
}

void NotepadBody::onOpen(PerformContext* ctx) {
    if (m_modified) {
        int ret = wxMessageBox("Save changes?", "Notepad", wxYES_NO | wxCANCEL | wxICON_QUESTION);
        if (ret == wxYES) {
            if (m_file)
                uiPersistObject(*m_file);
            else if (!uiSaveAs())
                return;
        } else if (ret == wxCANCEL)
            return;
    }
    if (!volumeManager()) {
        wxMessageBox("No volume manager", "Error", wxOK | wxICON_ERROR);
        return;
    }
    ChooseFileDialog dlg(m_frame, volumeManager(), "Open File", FileDialogMode::Open, "/", "");
    dlg.addFilter("Text files", "*.txt");
    dlg.addFilter("All files", "*.*");
    dlg.setFileMustExist(true);
    if (dlg.ShowModal() == wxID_OK) {
        auto vf = dlg.getVolumeFile();
        if (!vf)
            return;
        m_file = *vf;
        m_loaded = true;
        m_text->SetValue(m_file->readFileString(m_encoding.ToStdString()));
    }
}

void NotepadBody::onSave(PerformContext* ctx) {
    if (!m_file) {
        uiSaveAs();
    } else {
        uiPersistObject(*m_file);
    }
}

void NotepadBody::onSaveAs(PerformContext* ctx) { uiSaveAs(); }

void NotepadBody::onUndo(PerformContext* ctx) { m_text->Undo(); }

void NotepadBody::onRedo(PerformContext* ctx) { m_text->Redo(); }

void NotepadBody::onSelectAll(PerformContext* ctx) { m_text->SelectAll(); }

void NotepadBody::onClear(PerformContext* ctx) { m_text->Clear(); }

void NotepadBody::onCut(PerformContext* ctx) { m_text->Cut(); }

void NotepadBody::onCopy(PerformContext* ctx) { m_text->Copy(); }

void NotepadBody::onPaste(PerformContext* ctx) { m_text->Paste(); }

void NotepadBody::onZoomIn(PerformContext* ctx) {
    int fontSize = m_text->GetFont().GetPointSize();
    m_text->SetFont(
        wxFont(fontSize + 1, wxFONTFAMILY_DEFAULT, wxFONTWEIGHT_NORMAL, wxFONTSTYLE_NORMAL));
}

void NotepadBody::onZoomOut(PerformContext* ctx) {
    int fontSize = m_text->GetFont().GetPointSize();
    m_text->SetFont(
        wxFont(fontSize - 1, wxFONTFAMILY_DEFAULT, wxFONTWEIGHT_NORMAL, wxFONTSTYLE_NORMAL));
}

void NotepadBody::onZoomReset(PerformContext* ctx) {
    m_text->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTWEIGHT_NORMAL, wxFONTSTYLE_NORMAL));
}

void NotepadBody::onFind(PerformContext* ctx) {
    wxTextEntryDialog findDlg(m_frame, "Find what:", "Find");
    if (findDlg.ShowModal() != wxID_OK)
        return;
    wxString needle = findDlg.GetValue();
    if (needle.IsEmpty())
        return;

    wxString text = m_text->GetValue();
    long start = m_text->GetInsertionPoint();
    if (start < 0)
        start = 0;

    int pos = text.find(needle, (size_t)start);
    if (pos == wxNOT_FOUND && start > 0) {
        // wrap
        pos = text.find(needle, 0);
    }
    if (pos == wxNOT_FOUND) {
        wxMessageBox("Not found", "Find", wxOK | wxICON_INFORMATION);
        return;
    }
    m_text->SetFocus();
    m_text->SetSelection(pos, pos + needle.Length());
    m_text->ShowPosition(pos);
}

void NotepadBody::onReplace(PerformContext* ctx) {
    wxTextEntryDialog findDlg(m_frame, "Find what:", "Replace");
    if (findDlg.ShowModal() != wxID_OK)
        return;
    wxString find = findDlg.GetValue();
    if (find.IsEmpty())
        return;

    wxTextEntryDialog repDlg(m_frame, "Replace with:", "Replace");
    if (repDlg.ShowModal() != wxID_OK)
        return;
    wxString rep = repDlg.GetValue();

    wxString text = m_text->GetValue();
    text.Replace(find, rep, true);
    m_text->SetValue(text);
    m_modified = true;
}

void NotepadBody::onEncoding(PerformContext* ctx) {
    wxArrayString choices;
    choices.Add("UTF-8");
    choices.Add("ASCII");
    choices.Add("ISO-8859-1");
    wxSingleChoiceDialog dlg(m_frame, "Select encoding (applied on next Save/Open):", "Encoding",
                             choices);
    if (dlg.ShowModal() != wxID_OK)
        return;
    m_encoding = dlg.GetStringSelection();
}

} // namespace os

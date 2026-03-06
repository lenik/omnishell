#include "ChooseFolderDialog.hpp"

#include <bas/volume/DirEntry.hpp>
#include <bas/volume/FileStatus.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

namespace os {

ChooseFolderDialog::ChooseFolderDialog(
    wxWindow* parent,
    VolumeManager* volumeManager,
    const wxString& title,
    const wxString& message,
    const wxString& defaultPath
)
    : wxDialog(parent, wxID_ANY, title,
               wxDefaultPosition, wxSize(550, 450),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , volumeManager_(volumeManager)
    , selectedVolumeIndex_(0)
    , showNewFolder_(true)
    , mustExist_(true)
{
    currentPath_ = defaultPath.ToStdString();
    if (currentPath_.empty() || currentPath_[0] != '/') currentPath_ = "/";
    CreateControls();
    if (!message.IsEmpty()) {
        messageText_->SetLabel(message);
    }
    RefreshDir();
}

ChooseFolderDialog::~ChooseFolderDialog() {
}

void ChooseFolderDialog::setPath(const wxString& path) {
    SetCurrentPath(path.ToStdString());
    RefreshDir();
}

VolumeFile ChooseFolderDialog::getVolumeFile() const {
    if (!volumeManager_ || selectedVolumeIndex_ < 0 ||
        selectedVolumeIndex_ >= (int)volumeManager_->getVolumeCount()) {
        return VolumeFile();
    }
    Volume* vol = volumeManager_->getVolume(selectedVolumeIndex_);
    if (!vol) return VolumeFile();
    try {
        std::string path = vol->normalize(currentPath_, false);
        return VolumeFile(vol, path);
    } catch (...) {
        return VolumeFile();
    }
}

wxString ChooseFolderDialog::getPath() const {
    return wxString(currentPath_);
}

void ChooseFolderDialog::setShowNewFolderButton(bool show) {
    showNewFolder_ = show;
}

void ChooseFolderDialog::setMustExist(bool mustExist) {
    mustExist_ = mustExist;
}

void ChooseFolderDialog::CreateControls() {
    wxBoxSizer* main = new wxBoxSizer(wxVERTICAL);

    messageText_ = new wxStaticText(this, wxID_ANY, "Choose a folder:");
    main->Add(messageText_, 0, wxALL, 5);

    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(this, wxID_ANY, "Volume:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    volumeCombo_ = new wxComboBox(this, wxID_ANY, wxString(), wxDefaultPosition, wxSize(200, -1), 0, nullptr, wxCB_READONLY);
    if (volumeManager_) {
        for (size_t i = 0; i < volumeManager_->getVolumeCount(); i++) {
            Volume* v = volumeManager_->getVolume(i);
            volumeCombo_->Append(wxString(v->getLabel().empty() ? v->getId() : v->getLabel()));
        }
        if (volumeManager_->getVolumeCount() > 0) volumeCombo_->SetSelection(0);
    }
    volumeCombo_->Bind(wxEVT_COMBOBOX, &ChooseFolderDialog::OnVolumeSelected, this);
    row->Add(volumeCombo_, 0, wxALL, 5);
    main->Add(row, 0, wxEXPAND);

    row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(this, wxID_ANY, "Path:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    pathText_ = new wxTextCtrl(this, wxID_ANY, wxString(currentPath_));
    pathText_->Bind(wxEVT_TEXT_ENTER, &ChooseFolderDialog::OnPathEnter, this);
    row->Add(pathText_, 1, wxALL | wxEXPAND, 5);
    main->Add(row, 0, wxEXPAND);

    listCtrl_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 200), wxLC_LIST | wxLC_SINGLE_SEL);
    listCtrl_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ChooseFolderDialog::OnItemActivated, this);
    main->Add(listCtrl_, 1, wxEXPAND | wxALL, 5);

    row = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
    wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    okBtn->Bind(wxEVT_BUTTON, &ChooseFolderDialog::OnOK, this);
    cancelBtn->Bind(wxEVT_BUTTON, &ChooseFolderDialog::OnCancel, this);
    row->Add(okBtn, 0, wxALL, 5);
    row->Add(cancelBtn, 0, wxALL, 5);
    if (showNewFolder_) {
        newFolderButton_ = new wxButton(this, wxID_ANY, "New Folder");
        newFolderButton_->Bind(wxEVT_BUTTON, &ChooseFolderDialog::OnNewFolder, this);
        row->Add(newFolderButton_, 0, wxALL, 5);
    }
    main->Add(row, 0, wxEXPAND | wxALL, 5);

    SetSizer(main);
    Layout();
}

void ChooseFolderDialog::SetCurrentPath(const std::string& path) {
    std::string p = path;
    if (p.empty()) p = "/";
    if (p.size() > 1 && p.back() == '/') p.pop_back();
    if (p[0] != '/') p = "/" + p;
    currentPath_ = p;
    pathText_->SetValue(wxString(p));
}

void ChooseFolderDialog::RefreshDir() {
    listCtrl_->DeleteAllItems();
    if (!volumeManager_ || selectedVolumeIndex_ < 0 ||
        selectedVolumeIndex_ >= (int)volumeManager_->getVolumeCount()) {
        return;
    }
    Volume* vol = volumeManager_->getVolume(selectedVolumeIndex_);
    if (!vol) return;
    try {
        std::string path = currentPath_.empty() ? "/" : currentPath_;
        auto entries = vol->readDir(path, false);
        for (size_t i = 0; i < entries.size(); i++) {
            const auto& e = entries[i];
            if (!e->isDirectory()) continue;
            wxString name = e->name + "/";
            listCtrl_->InsertItem(static_cast<long>(i), name);
        }
    } catch (const std::exception& ex) {
        wxLogError("readDir: %s", ex.what());
    }
}

void ChooseFolderDialog::OnOK(wxCommandEvent& event) {
    if (!volumeManager_ || selectedVolumeIndex_ >= (int)volumeManager_->getVolumeCount()) {
        wxMessageBox("No volume selected", "Error", wxOK | wxICON_ERROR);
        return;
    }
    Volume* vol = volumeManager_->getVolume(selectedVolumeIndex_);
    std::string path;
    try {
        path = vol->normalize(currentPath_, false);
    } catch (...) {
        wxMessageBox("Invalid path", "Error", wxOK | wxICON_ERROR);
        return;
    }
    if (mustExist_ && !vol->isDirectory(path)) {
        wxMessageBox("Folder does not exist", "Error", wxOK | wxICON_ERROR);
        return;
    }
    EndModal(wxID_OK);
}

void ChooseFolderDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void ChooseFolderDialog::OnNewFolder(wxCommandEvent& event) {
    wxTextEntryDialog dlg(this, "Enter folder name:", "New Folder", "New Folder");
    if (dlg.ShowModal() != wxID_OK) return;
    wxString name = dlg.GetValue().Trim();
    if (name.IsEmpty()) return;

    if (!volumeManager_ || selectedVolumeIndex_ >= (int)volumeManager_->getVolumeCount()) return;
    Volume* vol = volumeManager_->getVolume(selectedVolumeIndex_);
    std::string parent = currentPath_.empty() ? "/" : currentPath_;
    if (parent.back() != '/') parent += "/";
    parent += name.ToStdString();
    try {
        if (!vol->createDirectory(parent)) {
            wxMessageBox("Folder already exists or cannot create", "Error", wxOK | wxICON_ERROR);
            return;
        }
        SetCurrentPath(parent);
        RefreshDir();
    } catch (const std::exception& ex) {
        wxMessageBox(wxString("Failed to create folder: ") + ex.what(), "Error", wxOK | wxICON_ERROR);
    }
}

void ChooseFolderDialog::OnVolumeSelected(wxCommandEvent& event) {
    selectedVolumeIndex_ = volumeCombo_->GetSelection();
    SetCurrentPath("/");
    RefreshDir();
}

void ChooseFolderDialog::OnItemActivated(wxListEvent& event) {
    wxString name = listCtrl_->GetItemText(event.GetIndex());
    if (name.EndsWith("/")) name.RemoveLast();
    std::string np = currentPath_.empty() ? "/" : currentPath_;
    if (np.back() != '/') np += "/";
    np += name.ToStdString();
    SetCurrentPath(np);
    RefreshDir();
}

void ChooseFolderDialog::OnPathEnter(wxCommandEvent& event) {
    SetCurrentPath(pathText_->GetValue().ToStdString());
    RefreshDir();
}

} // namespace os

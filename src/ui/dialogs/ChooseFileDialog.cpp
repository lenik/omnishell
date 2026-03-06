#include "ChooseFileDialog.hpp"

#include <bas/volume/DirEntry.hpp>
#include <bas/volume/FileStatus.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/filename.h>
#include <wx/log.h>

#include <algorithm>
#include <cstring>

namespace os {

ChooseFileDialog::ChooseFileDialog(
    wxWindow* parent,
    VolumeManager* volumeManager,
    const wxString& title,
    FileDialogMode mode,
    const wxString& defaultPath,
    const wxString& defaultFile
)
    : wxDialog(parent, wxID_ANY, title,
               wxDefaultPosition, wxSize(600, 500),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , volumeManager_(volumeManager)
    , mode_(mode)
    , multiSelect_(false)
    , fileMustExist_(mode != FileDialogMode::Save)
    , selectedVolumeIndex_(0)
    , filterIndex_(0)
{
    currentPath_ = defaultPath.ToStdString();
    if (currentPath_.empty() || currentPath_[0] != '/') currentPath_ = "/";
    CreateControls();
    if (!defaultFile.IsEmpty()) {
        filenameText_->SetValue(defaultFile);
    }
    if (filters_.empty()) {
        addFilter("All Files", "*.*");
    }
    RefreshDir();
}

ChooseFileDialog::~ChooseFileDialog() {
}

void ChooseFileDialog::addFilter(const wxString& description, const wxString& pattern) {
    Filter f;
    f.description = description;
    f.pattern = pattern;
    filters_.push_back(f);
    filterCombo_->Append(description);
    if (filters_.size() == 1) filterCombo_->SetSelection(0);
}

void ChooseFileDialog::setMultiSelect(bool enable) {
    multiSelect_ = enable;
}

void ChooseFileDialog::setFileMustExist(bool mustExist) {
    fileMustExist_ = mustExist;
}

VolumeFile ChooseFileDialog::getVolumeFile() const {
    auto vfs = getVolumeFiles();
    return vfs.empty() ? VolumeFile() : vfs[0];
}

wxString ChooseFileDialog::getPath() const {
    VolumeFile vf = getVolumeFile();
    if (vf.isEmpty()) return wxString();
    return wxString(vf.getPath());
}

std::vector<VolumeFile> ChooseFileDialog::getVolumeFiles() const {
    std::vector<VolumeFile> result;
    if (!volumeManager_ || selectedVolumeIndex_ < 0 ||
        selectedVolumeIndex_ >= (int)volumeManager_->getVolumeCount()) {
        return result;
    }
    Volume* vol = volumeManager_->getVolume(selectedVolumeIndex_);
    if (!vol) return result;

    wxString fn = filenameText_->GetValue().Trim();
    if (fn.IsEmpty()) return result;

    std::string path = currentPath_;
    if (path.empty() || path.back() != '/') path += "/";
    path += fn.ToStdString();

    try {
        path = vol->normalize(path, false);
    } catch (...) {
        return result;
    }

    if (mode_ == FileDialogMode::Save) {
        result.push_back(VolumeFile(vol, path));
        return result;
    }
    if (fileMustExist_ && !vol->exists(path)) return result;
    result.push_back(VolumeFile(vol, path));
    return result;
}

int ChooseFileDialog::getFilterIndex() const {
    return filterCombo_->GetSelection();
}

wxString ChooseFileDialog::getFilename() const {
    return filenameText_->GetValue();
}

wxString ChooseFileDialog::getDirectory() const {
    return wxString(currentPath_);
}

void ChooseFileDialog::CreateControls() {
    wxBoxSizer* main = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "Volume:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    volumeCombo_ = new wxComboBox(this, wxID_ANY, wxString(), wxDefaultPosition, wxSize(200, -1), 0, nullptr, wxCB_READONLY);
    if (volumeManager_) {
        for (size_t i = 0; i < volumeManager_->getVolumeCount(); i++) {
            Volume* v = volumeManager_->getVolume(i);
            volumeCombo_->Append(wxString(v->getLabel().empty() ? v->getId() : v->getLabel()));
        }
        if (volumeManager_->getVolumeCount() > 0) {
            volumeCombo_->SetSelection(0);
        }
    }
    volumeCombo_->Bind(wxEVT_COMBOBOX, &ChooseFileDialog::OnVolumeSelected, this);
    row1->Add(volumeCombo_, 0, wxALL, 5);
    main->Add(row1, 0, wxEXPAND);

    row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "Path:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    pathText_ = new wxTextCtrl(this, wxID_ANY, wxString(currentPath_));
    pathText_->Bind(wxEVT_TEXT_ENTER, &ChooseFileDialog::OnPathEnter, this);
    row1->Add(pathText_, 1, wxALL | wxEXPAND, 5);
    main->Add(row1, 0, wxEXPAND);

    listCtrl_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 220),
                               wxLC_LIST | wxLC_SINGLE_SEL);
    listCtrl_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ChooseFileDialog::OnItemActivated, this);
    listCtrl_->Bind(wxEVT_LIST_ITEM_SELECTED, &ChooseFileDialog::OnItemSelected, this);
    main->Add(listCtrl_, 1, wxEXPAND | wxALL, 5);

    row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "File name:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    filenameText_ = new wxTextCtrl(this, wxID_ANY, wxString());
    row1->Add(filenameText_, 1, wxALL | wxEXPAND, 5);
    main->Add(row1, 0, wxEXPAND);

    row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "Files of type:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    filterCombo_ = new wxComboBox(this, wxID_ANY, wxString(), wxDefaultPosition, wxSize(150, -1), 0, nullptr, wxCB_READONLY);
    filterCombo_->Bind(wxEVT_COMBOBOX, &ChooseFileDialog::OnFilterChanged, this);
    row1->Add(filterCombo_, 0, wxALL, 5);
    main->Add(row1, 0, wxEXPAND);

    statusText_ = new wxStaticText(this, wxID_ANY, wxString());
    main->Add(statusText_, 0, wxALL, 5);

    wxSizer* btn = CreateButtonSizer(wxOK | wxCANCEL);
    main->Add(btn, 0, wxEXPAND | wxALL, 5);

    SetSizer(main);
    Layout();
}

void ChooseFileDialog::SetCurrentPath(const std::string& path) {
    std::string p = path;
    if (p.empty()) p = "/";
    if (p.back() == '/' && p.size() > 1) p.pop_back();
    if (p[0] != '/') p = "/" + p;
    currentPath_ = p;
    pathText_->SetValue(wxString(p));
}

void ChooseFileDialog::RefreshDir() {
    listCtrl_->DeleteAllItems();
    if (!volumeManager_ || selectedVolumeIndex_ < 0 ||
        selectedVolumeIndex_ >= (int)volumeManager_->getVolumeCount()) {
        return;
    }
    Volume* vol = volumeManager_->getVolume(selectedVolumeIndex_);
    if (!vol) return;

    try {
        std::string path = currentPath_;
        if (path.empty()) path = "/";
        auto entries = vol->readDir(path, false);
        for (size_t i = 0; i < entries.size(); i++) {
            const auto& e = entries[i];
            wxString name = e->name;
            if (e->isDirectory()) {
                name += "/";
            }
            listCtrl_->InsertItem(static_cast<long>(i), name);
        }
    } catch (const std::exception& ex) {
        wxLogError("readDir: %s", ex.what());
    }
}

bool ChooseFileDialog::matchesFilter(const std::string& name) const {
    if (filterIndex_ < 0 || filterIndex_ >= (int)filters_.size()) return true;
    const std::string& pattern = filters_[filterIndex_].pattern.ToStdString();
    if (pattern == "*.*" || pattern == "*") return true;
    size_t dot = name.rfind('.');
    std::string ext = dot != std::string::npos ? name.substr(dot) : "";
    if (pattern.find('*') != std::string::npos) {
        std::string want = pattern.substr(pattern.find('.'));
        return ext == want || want == ".*";
    }
    return name.size() >= pattern.size() &&
           name.compare(name.size() - pattern.size(), pattern.size(), pattern) == 0;
}

void ChooseFileDialog::OnOK(wxCommandEvent& event) {
    wxString fn = filenameText_->GetValue().Trim();
    if (fn.IsEmpty()) {
        wxMessageBox("Please enter a file name", "Error", wxOK | wxICON_ERROR);
        return;
    }
    std::string path = currentPath_;
    if (path.empty() || path.back() != '/') path += "/";
    path += fn.ToStdString();

    if (!volumeManager_ || selectedVolumeIndex_ >= (int)volumeManager_->getVolumeCount()) {
        wxMessageBox("No volume selected", "Error", wxOK | wxICON_ERROR);
        return;
    }
    Volume* vol = volumeManager_->getVolume(selectedVolumeIndex_);
    try {
        path = vol->normalize(path, false);
    } catch (...) {
        wxMessageBox("Invalid path", "Error", wxOK | wxICON_ERROR);
        return;
    }
    if (mode_ == FileDialogMode::Open && fileMustExist_ && !vol->exists(path)) {
        wxMessageBox("File does not exist", "Error", wxOK | wxICON_ERROR);
        return;
    }
    if (mode_ == FileDialogMode::Save && vol->exists(path) && vol->isFile(path)) {
        if (wxMessageBox("File exists. Overwrite?", "Confirm", wxYES_NO | wxICON_QUESTION) != wxYES) {
            return;
        }
    }
    EndModal(wxID_OK);
}

void ChooseFileDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void ChooseFileDialog::OnVolumeSelected(wxCommandEvent& event) {
    selectedVolumeIndex_ = volumeCombo_->GetSelection();
    SetCurrentPath("/");
    RefreshDir();
}

void ChooseFileDialog::OnItemActivated(wxListEvent& event) {
    long i = event.GetIndex();
    wxString name = listCtrl_->GetItemText(i);
    if (name.EndsWith("/")) {
        name.RemoveLast();
        std::string np = currentPath_;
        if (np.empty() || np.back() != '/') np += "/";
        np += name.ToStdString();
        SetCurrentPath(np);
        RefreshDir();
    } else {
        filenameText_->SetValue(name);
    }
}

void ChooseFileDialog::OnItemSelected(wxListEvent& event) {
    wxString name = listCtrl_->GetItemText(event.GetIndex());
    if (!name.EndsWith("/")) {
        filenameText_->SetValue(name);
    }
}

void ChooseFileDialog::OnFilterChanged(wxCommandEvent& event) {
    filterIndex_ = filterCombo_->GetSelection();
}

void ChooseFileDialog::OnPathEnter(wxCommandEvent& event) {
    SetCurrentPath(pathText_->GetValue().ToStdString());
    RefreshDir();
}

} // namespace os

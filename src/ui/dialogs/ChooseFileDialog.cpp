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
    , m_volumeManager(volumeManager)
    , m_mode(mode)
    , m_multiSelect(false)
    , m_fileMustExist(mode != FileDialogMode::Save)
    , m_selectedVolumeIndex(0)
    , m_filterIndex(0)
{
    m_currentPath = defaultPath.ToStdString();
    if (m_currentPath.empty() || m_currentPath[0] != '/') m_currentPath = "/";
    CreateControls();
    if (!defaultFile.IsEmpty()) {
        m_filenameText->SetValue(defaultFile);
    }
    if (m_filters.empty()) {
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
    m_filters.push_back(f);
    m_filterCombo->Append(description);
    if (m_filters.size() == 1) m_filterCombo->SetSelection(0);
}

void ChooseFileDialog::setMultiSelect(bool enable) {
    m_multiSelect = enable;
}

void ChooseFileDialog::setFileMustExist(bool mustExist) {
    m_fileMustExist = mustExist;
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
    if (!m_volumeManager || m_selectedVolumeIndex < 0 ||
        m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) {
        return result;
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    if (!vol) return result;

    wxString fn = m_filenameText->GetValue().Trim();
    if (fn.IsEmpty()) return result;

    std::string path = m_currentPath;
    if (path.empty() || path.back() != '/') path += "/";
    path += fn.ToStdString();

    try {
        path = vol->normalize(path, false);
    } catch (...) {
        return result;
    }

    if (m_mode == FileDialogMode::Save) {
        result.push_back(VolumeFile(vol, path));
        return result;
    }
    if (m_fileMustExist && !vol->exists(path)) return result;
    result.push_back(VolumeFile(vol, path));
    return result;
}

int ChooseFileDialog::getFilterIndex() const {
    return m_filterCombo->GetSelection();
}

wxString ChooseFileDialog::getFilename() const {
    return m_filenameText->GetValue();
}

wxString ChooseFileDialog::getDirectory() const {
    return wxString(m_currentPath);
}

void ChooseFileDialog::CreateControls() {
    wxBoxSizer* main = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "Volume:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_volumeCombo = new wxComboBox(this, wxID_ANY, wxString(), wxDefaultPosition, wxSize(200, -1), 0, nullptr, wxCB_READONLY);
    if (m_volumeManager) {
        for (size_t i = 0; i < m_volumeManager->getVolumeCount(); i++) {
            Volume* v = m_volumeManager->getVolume(i);
            m_volumeCombo->Append(wxString(v->getLabel().empty() ? v->getId() : v->getLabel()));
        }
        if (m_volumeManager->getVolumeCount() > 0) {
            m_volumeCombo->SetSelection(0);
        }
    }
    m_volumeCombo->Bind(wxEVT_COMBOBOX, &ChooseFileDialog::OnVolumeSelected, this);
    row1->Add(m_volumeCombo, 0, wxALL, 5);
    main->Add(row1, 0, wxEXPAND);

    row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "Path:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_pathText = new wxTextCtrl(this, wxID_ANY, wxString(m_currentPath));
    m_pathText->Bind(wxEVT_TEXT_ENTER, &ChooseFileDialog::OnPathEnter, this);
    row1->Add(m_pathText, 1, wxALL | wxEXPAND, 5);
    main->Add(row1, 0, wxEXPAND);

    m_listCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 220),
                               wxLC_LIST | wxLC_SINGLE_SEL);
    m_listCtrl->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ChooseFileDialog::OnItemActivated, this);
    m_listCtrl->Bind(wxEVT_LIST_ITEM_SELECTED, &ChooseFileDialog::OnItemSelected, this);
    main->Add(m_listCtrl, 1, wxEXPAND | wxALL, 5);

    row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "File name:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_filenameText = new wxTextCtrl(this, wxID_ANY, wxString());
    row1->Add(m_filenameText, 1, wxALL | wxEXPAND, 5);
    main->Add(row1, 0, wxEXPAND);

    row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "Files of type:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_filterCombo = new wxComboBox(this, wxID_ANY, wxString(), wxDefaultPosition, wxSize(150, -1), 0, nullptr, wxCB_READONLY);
    m_filterCombo->Bind(wxEVT_COMBOBOX, &ChooseFileDialog::OnFilterChanged, this);
    row1->Add(m_filterCombo, 0, wxALL, 5);
    main->Add(row1, 0, wxEXPAND);

    m_statusText = new wxStaticText(this, wxID_ANY, wxString());
    main->Add(m_statusText, 0, wxALL, 5);

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
    m_currentPath = p;
    m_pathText->SetValue(wxString(p));
}

void ChooseFileDialog::RefreshDir() {
    m_listCtrl->DeleteAllItems();
    if (!m_volumeManager || m_selectedVolumeIndex < 0 ||
        m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) {
        return;
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    if (!vol) return;

    try {
        std::string path = m_currentPath;
        if (path.empty()) path = "/";
        auto entries = vol->readDir(path, false);
        for (size_t i = 0; i < entries.size(); i++) {
            const auto& e = entries[i];
            wxString name = e->name;
            if (e->isDirectory()) {
                name += "/";
            }
            m_listCtrl->InsertItem(static_cast<long>(i), name);
        }
    } catch (const std::exception& ex) {
        wxLogError("readDir: %s", ex.what());
    }
}

bool ChooseFileDialog::matchesFilter(const std::string& name) const {
    if (m_filterIndex < 0 || m_filterIndex >= (int)m_filters.size()) return true;
    const std::string& pattern = m_filters[m_filterIndex].pattern.ToStdString();
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
    wxString fn = m_filenameText->GetValue().Trim();
    if (fn.IsEmpty()) {
        wxMessageBox("Please enter a file name", "Error", wxOK | wxICON_ERROR);
        return;
    }
    std::string path = m_currentPath;
    if (path.empty() || path.back() != '/') path += "/";
    path += fn.ToStdString();

    if (!m_volumeManager || m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) {
        wxMessageBox("No volume selected", "Error", wxOK | wxICON_ERROR);
        return;
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    try {
        path = vol->normalize(path, false);
    } catch (...) {
        wxMessageBox("Invalid path", "Error", wxOK | wxICON_ERROR);
        return;
    }
    if (m_mode == FileDialogMode::Open && m_fileMustExist && !vol->exists(path)) {
        wxMessageBox("File does not exist", "Error", wxOK | wxICON_ERROR);
        return;
    }
    if (m_mode == FileDialogMode::Save && vol->exists(path) && vol->isFile(path)) {
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
    m_selectedVolumeIndex = m_volumeCombo->GetSelection();
    SetCurrentPath("/");
    RefreshDir();
}

void ChooseFileDialog::OnItemActivated(wxListEvent& event) {
    long i = event.GetIndex();
    wxString name = m_listCtrl->GetItemText(i);
    if (name.EndsWith("/")) {
        name.RemoveLast();
        std::string np = m_currentPath;
        if (np.empty() || np.back() != '/') np += "/";
        np += name.ToStdString();
        SetCurrentPath(np);
        RefreshDir();
    } else {
        m_filenameText->SetValue(name);
    }
}

void ChooseFileDialog::OnItemSelected(wxListEvent& event) {
    wxString name = m_listCtrl->GetItemText(event.GetIndex());
    if (!name.EndsWith("/")) {
        m_filenameText->SetValue(name);
    }
}

void ChooseFileDialog::OnFilterChanged(wxCommandEvent& event) {
    m_filterIndex = m_filterCombo->GetSelection();
}

void ChooseFileDialog::OnPathEnter(wxCommandEvent& event) {
    SetCurrentPath(m_pathText->GetValue().ToStdString());
    RefreshDir();
}

} // namespace os

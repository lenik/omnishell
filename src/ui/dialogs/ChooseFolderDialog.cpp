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
    , m_volumeManager(volumeManager)
    , m_selectedVolumeIndex(0)
    , m_showNewFolder(true)
    , m_mustExist(true)
{
    m_currentPath = defaultPath.ToStdString();
    if (m_currentPath.empty() || m_currentPath[0] != '/') m_currentPath = "/";
    CreateControls();
    if (!message.IsEmpty()) {
        m_messageText->SetLabel(message);
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
    if (!m_volumeManager || m_selectedVolumeIndex < 0 ||
        m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) {
        return VolumeFile();
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    if (!vol) return VolumeFile();
    try {
        std::string path = vol->normalize(m_currentPath, false);
        return VolumeFile(vol, path);
    } catch (...) {
        return VolumeFile();
    }
}

wxString ChooseFolderDialog::getPath() const {
    return wxString(m_currentPath);
}

void ChooseFolderDialog::setShowNewFolderButton(bool show) {
    m_showNewFolder = show;
}

void ChooseFolderDialog::setMustExist(bool mustExist) {
    m_mustExist = mustExist;
}

void ChooseFolderDialog::CreateControls() {
    wxBoxSizer* main = new wxBoxSizer(wxVERTICAL);

    m_messageText = new wxStaticText(this, wxID_ANY, "Choose a folder:");
    main->Add(m_messageText, 0, wxALL, 5);

    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(this, wxID_ANY, "Volume:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_volumeCombo = new wxComboBox(this, wxID_ANY, wxString(), wxDefaultPosition, wxSize(200, -1), 0, nullptr, wxCB_READONLY);
    if (m_volumeManager) {
        for (size_t i = 0; i < m_volumeManager->getVolumeCount(); i++) {
            Volume* v = m_volumeManager->getVolume(i);
            m_volumeCombo->Append(wxString(v->getLabel().empty() ? v->getId() : v->getLabel()));
        }
        if (m_volumeManager->getVolumeCount() > 0) m_volumeCombo->SetSelection(0);
    }
    m_volumeCombo->Bind(wxEVT_COMBOBOX, &ChooseFolderDialog::OnVolumeSelected, this);
    row->Add(m_volumeCombo, 0, wxALL, 5);
    main->Add(row, 0, wxEXPAND);

    row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(this, wxID_ANY, "Path:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_pathText = new wxTextCtrl(this, wxID_ANY, wxString(m_currentPath));
    m_pathText->Bind(wxEVT_TEXT_ENTER, &ChooseFolderDialog::OnPathEnter, this);
    row->Add(m_pathText, 1, wxALL | wxEXPAND, 5);
    main->Add(row, 0, wxEXPAND);

    m_listCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 200), wxLC_LIST | wxLC_SINGLE_SEL);
    m_listCtrl->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ChooseFolderDialog::OnItemActivated, this);
    main->Add(m_listCtrl, 1, wxEXPAND | wxALL, 5);

    row = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
    wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    okBtn->Bind(wxEVT_BUTTON, &ChooseFolderDialog::OnOK, this);
    cancelBtn->Bind(wxEVT_BUTTON, &ChooseFolderDialog::OnCancel, this);
    row->Add(okBtn, 0, wxALL, 5);
    row->Add(cancelBtn, 0, wxALL, 5);
    if (m_showNewFolder) {
        m_newFolderButton = new wxButton(this, wxID_ANY, "New Folder");
        m_newFolderButton->Bind(wxEVT_BUTTON, &ChooseFolderDialog::OnNewFolder, this);
        row->Add(m_newFolderButton, 0, wxALL, 5);
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
    m_currentPath = p;
    m_pathText->SetValue(wxString(p));
}

void ChooseFolderDialog::RefreshDir() {
    m_listCtrl->DeleteAllItems();
    if (!m_volumeManager || m_selectedVolumeIndex < 0 ||
        m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) {
        return;
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    if (!vol) return;
    try {
        std::string path = m_currentPath.empty() ? "/" : m_currentPath;
        auto entries = vol->readDir(path, false);
        for (size_t i = 0; i < entries.size(); i++) {
            const auto& e = entries[i];
            if (!e->isDirectory()) continue;
            wxString name = e->name + "/";
            m_listCtrl->InsertItem(static_cast<long>(i), name);
        }
    } catch (const std::exception& ex) {
        wxLogError("readDir: %s", ex.what());
    }
}

void ChooseFolderDialog::OnOK(wxCommandEvent& event) {
    if (!m_volumeManager || m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) {
        wxMessageBox("No volume selected", "Error", wxOK | wxICON_ERROR);
        return;
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    std::string path;
    try {
        path = vol->normalize(m_currentPath, false);
    } catch (...) {
        wxMessageBox("Invalid path", "Error", wxOK | wxICON_ERROR);
        return;
    }
    if (m_mustExist && !vol->isDirectory(path)) {
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

    if (!m_volumeManager || m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) return;
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    std::string parent = m_currentPath.empty() ? "/" : m_currentPath;
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
    m_selectedVolumeIndex = m_volumeCombo->GetSelection();
    SetCurrentPath("/");
    RefreshDir();
}

void ChooseFolderDialog::OnItemActivated(wxListEvent& event) {
    wxString name = m_listCtrl->GetItemText(event.GetIndex());
    if (name.EndsWith("/")) name.RemoveLast();
    std::string np = m_currentPath.empty() ? "/" : m_currentPath;
    if (np.back() != '/') np += "/";
    np += name.ToStdString();
    SetCurrentPath(np);
    RefreshDir();
}

void ChooseFolderDialog::OnPathEnter(wxCommandEvent& event) {
    SetCurrentPath(m_pathText->GetValue().ToStdString());
    RefreshDir();
}

} // namespace os

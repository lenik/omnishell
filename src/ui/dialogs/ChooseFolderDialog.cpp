#include "ChooseFolderDialog.hpp"

#include "../Location.hpp"

#include <bas/volume/DirEntry.hpp>
#include <bas/volume/FileStatus.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#include <optional>

namespace os {

ChooseFolderDialog::ChooseFolderDialog(
    wxWindow* parent,
    VolumeManager* volumeManager,
    const wxString& title,
    const wxString& message,
    const wxString& defaultPath
)
    : wxcDialog(parent, wxID_ANY, title,
               wxDefaultPosition, wxSize(520, 480),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_volumeManager(volumeManager)
    , m_selectedVolumeIndex(0)
    , m_showNewFolder(true)
    , m_mustExist(true) {
    m_currentPath = defaultPath.ToStdString();
    if (m_currentPath.empty() || m_currentPath[0] != '/')
        m_currentPath = "/";
    CreateControls();
    if (!message.IsEmpty()) {
        m_messageText->SetLabel(message);
    }
    syncTree();
}

ChooseFolderDialog::~ChooseFolderDialog() = default;

void ChooseFolderDialog::setPath(const wxString& path) {
    SetCurrentPath(path.ToStdString());
    syncTree();
}

std::optional<VolumeFile> ChooseFolderDialog::getVolumeFile() const {
    if (!m_volumeManager || m_selectedVolumeIndex < 0 ||
        m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) {
        return std::nullopt;
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    if (!vol)
        return std::nullopt;
    try {
        std::string path = vol->normalize(m_currentPath, false);
        return VolumeFile(vol, path);
    } catch (...) {
        return std::nullopt;
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
        if (m_volumeManager->getVolumeCount() > 0)
            m_volumeCombo->SetSelection(0);
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

    if (m_volumeManager && m_volumeManager->getVolumeCount() > 0) {
        Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
        if (vol) {
            Location loc(vol, normalizePath(m_currentPath));
            m_tree = new DirTreeView(this, loc);
            m_tree->onLocationChange([this](const Location& loc) { SetCurrentPath(loc.path); });
            main->Add(m_tree, 1, wxEXPAND | wxALL, 5);
        }
    }
    if (!m_tree)
        main->Add(new wxStaticText(this, wxID_ANY, "No volumes available."), 1, wxEXPAND | wxALL, 8);

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
    if (p.empty())
        p = "/";
    if (p.size() > 1 && p.back() == '/')
        p.pop_back();
    if (p[0] != '/')
        p = "/" + p;
    if (m_volumeManager && m_selectedVolumeIndex >= 0 &&
        m_selectedVolumeIndex < (int)m_volumeManager->getVolumeCount()) {
        Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
        if (vol) {
            try {
                p = vol->normalize(p, false);
            } catch (...) {
            }
        }
    }
    m_currentPath = p;
    if (m_pathText)
        m_pathText->SetValue(wxString(m_currentPath));
}

void ChooseFolderDialog::syncTree() {
    if (!m_volumeManager || m_selectedVolumeIndex < 0 ||
        m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount() || !m_tree) {
        return;
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    if (!vol)
        return;
    m_tree->setLocation(Location(vol, normalizePath(m_currentPath)));
    m_pathText->SetValue(wxString(m_currentPath));
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
    if (dlg.ShowModal() != wxID_OK)
        return;
    wxString name = dlg.GetValue().Trim();
    if (name.IsEmpty())
        return;

    if (!m_volumeManager || m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount())
        return;
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    std::string parent = m_currentPath.empty() ? "/" : m_currentPath;
    if (parent.back() != '/')
        parent += "/";
    parent += name.ToStdString();
    try {
        if (!vol->createDirectory(parent)) {
            wxMessageBox("Folder already exists or cannot create", "Error", wxOK | wxICON_ERROR);
            return;
        }
        SetCurrentPath(parent);
        syncTree();
    } catch (const std::exception& ex) {
        wxMessageBox(wxString("Failed to create folder: ") + ex.what(), "Error", wxOK | wxICON_ERROR);
    }
}

void ChooseFolderDialog::OnVolumeSelected(wxCommandEvent& event) {
    m_selectedVolumeIndex = m_volumeCombo->GetSelection();
    SetCurrentPath("/");
    syncTree();
}

void ChooseFolderDialog::OnPathEnter(wxCommandEvent& event) {
    SetCurrentPath(m_pathText->GetValue().ToStdString());
    syncTree();
}

} // namespace os

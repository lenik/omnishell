#include "ChooseFileDialog.hpp"

#include "../Location.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/filename.h>
#include <wx/log.h>

#include <optional>

namespace os {

ChooseFileDialog::ChooseFileDialog(wxWindow* parent, VolumeManager* volumeManager,
                                   const wxString& title, FileDialogMode mode,
                                   const wxString& defaultPath, const wxString& defaultFile)
    : wxcDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(720, 520),
                wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_volumeManager(volumeManager), m_mode(mode), m_multiSelect(false),
      m_fileMustExist(mode != FileDialogMode::Save), m_selectedVolumeIndex(0), m_filterIndex(0) {
    m_currentPath = defaultPath.ToStdString();
    if (m_currentPath.empty() || m_currentPath[0] != '/')
        m_currentPath = "/";
    CreateControls();
    if (!defaultFile.IsEmpty()) {
        m_filenameText->SetValue(defaultFile);
    }
    if (m_filters.empty()) {
        addFilter("All Files", "*.*");
    }
    syncViews();
}

ChooseFileDialog::~ChooseFileDialog() = default;

void ChooseFileDialog::addFilter(const wxString& description, const wxString& pattern) {
    Filter f;
    f.description = description;
    f.pattern = pattern;
    m_filters.push_back(f);
    m_filterCombo->Append(description);
    if (m_filters.size() == 1)
        m_filterCombo->SetSelection(0);
    if (m_list) {
        syncListFilter();
        m_list->refresh();
    }
}

void ChooseFileDialog::setMultiSelect(bool enable) { m_multiSelect = enable; }

void ChooseFileDialog::setFileMustExist(bool mustExist) { m_fileMustExist = mustExist; }

std::optional<VolumeFile> ChooseFileDialog::getVolumeFile() const {
    auto vfs = getVolumeFiles();
    if (vfs.empty())
        return std::nullopt;
    return vfs[0];
}

wxString ChooseFileDialog::getPath() const {
    auto vf = getVolumeFile();
    if (!vf)
        return wxString();
    return wxString(vf->getPath());
}

std::vector<VolumeFile> ChooseFileDialog::getVolumeFiles() const {
    std::vector<VolumeFile> result;
    if (!m_volumeManager || m_selectedVolumeIndex < 0 ||
        m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) {
        return result;
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    if (!vol)
        return result;

    wxString fn = m_filenameText->GetValue().Trim();
    if (fn.IsEmpty())
        return result;

    std::string path = m_currentPath;
    if (path.empty() || path.back() != '/')
        path += "/";
    path += fn.ToStdString();

    try {
        path = vol->normalizeArg(path);
    } catch (...) {
        return result;
    }

    if (m_mode == FileDialogMode::Save) {
        result.push_back(VolumeFile(vol, path));
        return result;
    }
    if (m_fileMustExist && !vol->exists(path))
        return result;
    result.push_back(VolumeFile(vol, path));
    return result;
}

int ChooseFileDialog::getFilterIndex() const { return m_filterCombo->GetSelection(); }

wxString ChooseFileDialog::getFilename() const { return m_filenameText->GetValue(); }

wxString ChooseFileDialog::getDirectory() const { return wxString(m_currentPath); }

void ChooseFileDialog::CreateControls() {
    wxBoxSizer* main = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "Volume:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_volumeCombo = new wxComboBox(this, wxID_ANY, wxString(), wxDefaultPosition, wxSize(200, -1),
                                   0, nullptr, wxCB_READONLY);
    if (m_volumeManager) {
        for (size_t i = 0; i < m_volumeManager->getVolumeCount(); i++) {
            Volume* v = m_volumeManager->getVolume(i);
            m_volumeCombo->Append(wxString(v->getLabel().empty() ? v->getUrl() : v->getLabel()));
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

    m_filenameText = new wxTextCtrl(this, wxID_ANY, wxString());
    m_filterCombo = new wxComboBox(this, wxID_ANY, wxString(), wxDefaultPosition, wxSize(150, -1),
                                   0, nullptr, wxCB_READONLY);
    m_filterCombo->Bind(wxEVT_COMBOBOX, &ChooseFileDialog::OnFilterChanged, this);

    if (m_volumeManager && m_volumeManager->getVolumeCount() > 0) {
        Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
        if (vol) {
            Location loc(vol, normalizePath(m_currentPath));
            m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 260),
                                              wxSP_3D | wxSP_LIVE_UPDATE);
            m_tree = new DirTreeView(m_splitter, loc);
            m_list = new FileListView(m_splitter, loc);
            m_splitter->SplitVertically(m_tree, m_list, 220);
            m_splitter->SetMinimumPaneSize(120);

            m_tree->onLocationChange([this](const Location& loc) {
                SetCurrentPath(loc.path);
                syncListFilter();
                if (m_list)
                    m_list->setLocation(loc);
            });

            m_list->onFileSelected([this](VolumeFile& vf) {
                if (!vf.isDirectory()) {
                    std::string p = vf.getPath();
                    size_t slash = p.rfind('/');
                    std::string name = slash != std::string::npos ? p.substr(slash + 1) : p;
                    m_filenameText->SetValue(wxString::FromUTF8(name.data(), name.size()));
                }
            });
            m_list->onFileActivated([this](VolumeFile& vf) {
                if (vf.isDirectory()) {
                    SetCurrentPath(vf.getPath());
                    syncViews();
                } else {
                    std::string p = vf.getPath();
                    size_t slash = p.rfind('/');
                    std::string name = slash != std::string::npos ? p.substr(slash + 1) : p;
                    m_filenameText->SetValue(wxString::FromUTF8(name.data(), name.size()));
                }
            });
        }
    }
    if (m_splitter)
        main->Add(m_splitter, 1, wxEXPAND | wxALL, 5);
    else
        main->Add(new wxStaticText(this, wxID_ANY, "No volumes available."), 1, wxEXPAND | wxALL,
                  8);

    row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "File name:"), 0, wxALL | wxALIGN_CENTER_VERTICAL,
              5);
    row1->Add(m_filenameText, 1, wxALL | wxEXPAND, 5);
    main->Add(row1, 0, wxEXPAND);

    row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(this, wxID_ANY, "Files of type:"), 0,
              wxALL | wxALIGN_CENTER_VERTICAL, 5);
    row1->Add(m_filterCombo, 0, wxALL, 5);
    main->Add(row1, 0, wxEXPAND);

    m_statusText = new wxStaticText(this, wxID_ANY, wxString());
    main->Add(m_statusText, 0, wxALL, 5);

    wxSizer* btn = CreateButtonSizer(wxOK | wxCANCEL);
    main->Add(btn, 0, wxEXPAND | wxALL, 5);

    Bind(wxEVT_BUTTON, &ChooseFileDialog::OnOK, this, wxID_OK);
    Bind(wxEVT_BUTTON, &ChooseFileDialog::OnCancel, this, wxID_CANCEL);

    SetSizer(main);
    Layout();
}

void ChooseFileDialog::syncListFilter() {
    if (!m_list)
        return;
    m_list->setEntryFilter(
        [this](const DirEntry& e) { return e.isDirectory() || matchesFilter(e.name); });
}

void ChooseFileDialog::syncViews() {
    if (!m_volumeManager || m_selectedVolumeIndex < 0 ||
        m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) {
        return;
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    if (!vol || !m_tree || !m_list)
        return;
    Location loc(vol, normalizePath(m_currentPath));
    syncListFilter();
    m_list->setLocation(loc);
    m_tree->setLocation(loc);
    m_pathText->SetValue(wxString(m_currentPath));
}

void ChooseFileDialog::SetCurrentPath(const std::string& path) {
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
                p = vol->normalizeArg(p);
            } catch (...) {
            }
        }
    }
    m_currentPath = p;
    if (m_pathText)
        m_pathText->SetValue(wxString(m_currentPath));
}

bool ChooseFileDialog::matchesFilter(const std::string& name) const {
    if (m_filterIndex < 0 || m_filterIndex >= (int)m_filters.size())
        return true;
    const std::string& pattern = m_filters[m_filterIndex].pattern.ToStdString();
    if (pattern == "*.*" || pattern == "*")
        return true;
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
    if (path.empty() || path.back() != '/')
        path += "/";
    path += fn.ToStdString();

    if (!m_volumeManager || m_selectedVolumeIndex >= (int)m_volumeManager->getVolumeCount()) {
        wxMessageBox("No volume selected", "Error", wxOK | wxICON_ERROR);
        return;
    }
    Volume* vol = m_volumeManager->getVolume(m_selectedVolumeIndex);
    try {
        path = vol->normalizeArg(path);
    } catch (...) {
        wxMessageBox("Invalid path", "Error", wxOK | wxICON_ERROR);
        return;
    }
    if (m_mode == FileDialogMode::Open && m_fileMustExist && !vol->exists(path)) {
        wxMessageBox("File does not exist", "Error", wxOK | wxICON_ERROR);
        return;
    }
    if (m_mode == FileDialogMode::Save && vol->exists(path) && vol->isFile(path)) {
        if (wxMessageBox("File exists. Overwrite?", "Confirm", wxYES_NO | wxICON_QUESTION) !=
            wxYES) {
            return;
        }
    }
    EndModal(wxID_OK);
}

void ChooseFileDialog::OnCancel(wxCommandEvent& event) { EndModal(wxID_CANCEL); }

void ChooseFileDialog::OnVolumeSelected(wxCommandEvent& event) {
    m_selectedVolumeIndex = m_volumeCombo->GetSelection();
    SetCurrentPath("/");
    syncViews();
}

void ChooseFileDialog::OnFilterChanged(wxCommandEvent& event) {
    m_filterIndex = m_filterCombo->GetSelection();
    if (m_list) {
        syncListFilter();
        m_list->refresh();
    }
}

void ChooseFileDialog::OnPathEnter(wxCommandEvent& event) {
    SetCurrentPath(m_pathText->GetValue().ToStdString());
    syncViews();
}

} // namespace os

#ifndef OMNISHELL_UI_CHOOSE_FILE_DIALOG_HPP
#define OMNISHELL_UI_CHOOSE_FILE_DIALOG_HPP

#include "../widget/DirTreeView.hpp"
#include "../widget/FileListView.hpp"
#include "../../wx/wxcWindow.hpp"

#include <wx/combobox.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <bas/volume/VolumeFile.hpp>

#include <optional>
#include <string>
#include <vector>

class VolumeManager;

namespace os {

enum class FileDialogMode {
    Open,
    Save,
};

/**
 * VFS-based file selection dialog.
 * Uses VolumeManager and Volume API only (no local filesystem).
 */
class ChooseFileDialog : public wxcDialog {
public:
    ChooseFileDialog(
        wxWindow* parent,
        VolumeManager* volumeManager,
        const wxString& title = "Select File",
        FileDialogMode mode = FileDialogMode::Open,
        const wxString& defaultPath = "/",
        const wxString& defaultFile = ""
    );
    virtual ~ChooseFileDialog();

    void addFilter(const wxString& description, const wxString& pattern);
    void setMultiSelect(bool enable);
    void setFileMustExist(bool mustExist);

    /** Selected file as VolumeFile (volume + path). Nullopt if cancelled or none. */
    std::optional<VolumeFile> getVolumeFile() const;
    wxString getPath() const;  // legacy: returns "volumeIndex:/path" or empty
    std::vector<VolumeFile> getVolumeFiles() const;
    int getFilterIndex() const;
    wxString getFilename() const;
    wxString getDirectory() const;

protected:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnVolumeSelected(wxCommandEvent& event);
    void OnFilterChanged(wxCommandEvent& event);
    void OnPathEnter(wxCommandEvent& event);

    void CreateControls();
    void syncListFilter();
    void syncViews();
    void SetCurrentPath(const std::string& path);
    bool matchesFilter(const std::string& name) const;

private:
    VolumeManager* m_volumeManager;
    wxComboBox* m_volumeCombo;
    wxTextCtrl* m_pathText;
    wxSplitterWindow* m_splitter = nullptr;
    DirTreeView* m_tree = nullptr;
    FileListView* m_list = nullptr;
    wxTextCtrl* m_filenameText;
    wxComboBox* m_filterCombo;
    wxStaticText* m_statusText;

    FileDialogMode m_mode;
    bool m_multiSelect;
    bool m_fileMustExist;
    int m_selectedVolumeIndex;
    std::string m_currentPath;
    std::vector<int> m_selectedIndices;

    struct Filter {
        wxString description;
        wxString pattern;
    };
    std::vector<Filter> m_filters;
    int m_filterIndex;
};

} // namespace os

#endif // OMNISHELL_UI_CHOOSE_FILE_DIALOG_HPP

#ifndef OMNISHELL_UI_CHOOSE_FILE_DIALOG_HPP
#define OMNISHELL_UI_CHOOSE_FILE_DIALOG_HPP

#include <wx/combobox.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <string>
#include <vector>

class VolumeManager;
struct VolumeFile;

namespace os {

enum class FileDialogMode {
    Open,
    Save,
};

/**
 * VFS-based file selection dialog.
 * Uses VolumeManager and Volume API only (no local filesystem).
 */
class ChooseFileDialog : public wxDialog {
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

    /** Selected file as VolumeFile (volume + path). Empty if cancelled or none. */
    VolumeFile getVolumeFile() const;
    wxString getPath() const;  // legacy: returns "volumeIndex:/path" or empty
    std::vector<VolumeFile> getVolumeFiles() const;
    int getFilterIndex() const;
    wxString getFilename() const;
    wxString getDirectory() const;

protected:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnVolumeSelected(wxCommandEvent& event);
    void OnItemActivated(wxListEvent& event);
    void OnItemSelected(wxListEvent& event);
    void OnFilterChanged(wxCommandEvent& event);
    void OnPathEnter(wxCommandEvent& event);

    void CreateControls();
    void RefreshDir();
    void SetCurrentPath(const std::string& path);
    bool matchesFilter(const std::string& name) const;

private:
    VolumeManager* volumeManager_;
    wxComboBox* volumeCombo_;
    wxTextCtrl* pathText_;
    wxListCtrl* listCtrl_;
    wxTextCtrl* filenameText_;
    wxComboBox* filterCombo_;
    wxStaticText* statusText_;

    FileDialogMode mode_;
    bool multiSelect_;
    bool fileMustExist_;
    int selectedVolumeIndex_;
    std::string currentPath_;
    std::vector<int> selectedIndices_;

    struct Filter {
        wxString description;
        wxString pattern;
    };
    std::vector<Filter> filters_;
    int filterIndex_;
};

} // namespace os

#endif // OMNISHELL_UI_CHOOSE_FILE_DIALOG_HPP

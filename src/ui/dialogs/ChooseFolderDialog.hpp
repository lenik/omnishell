#ifndef OMNISHELL_UI_CHOOSE_FOLDER_DIALOG_HPP
#define OMNISHELL_UI_CHOOSE_FOLDER_DIALOG_HPP

#include <wx/combobox.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <string>

class VolumeManager;
struct VolumeFile;

namespace os {

/**
 * VFS-based folder selection dialog.
 * Uses VolumeManager and Volume API only.
 */
class ChooseFolderDialog : public wxDialog {
public:
    ChooseFolderDialog(
        wxWindow* parent,
        VolumeManager* volumeManager,
        const wxString& title = "Select Folder",
        const wxString& message = "Choose a folder:",
        const wxString& defaultPath = "/"
    );
    virtual ~ChooseFolderDialog();

    void setPath(const wxString& path);
    /** Selected folder as VolumeFile. Empty if cancelled. */
    VolumeFile getVolumeFile() const;
    wxString getPath() const;
    void setShowNewFolderButton(bool show);
    void setMustExist(bool mustExist);

protected:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnNewFolder(wxCommandEvent& event);
    void OnVolumeSelected(wxCommandEvent& event);
    void OnItemActivated(wxListEvent& event);
    void OnPathEnter(wxCommandEvent& event);

    void CreateControls();
    void RefreshDir();
    void SetCurrentPath(const std::string& path);

private:
    VolumeManager* volumeManager_;
    wxStaticText* messageText_;
    wxComboBox* volumeCombo_;
    wxTextCtrl* pathText_;
    wxListCtrl* listCtrl_;
    wxButton* newFolderButton_;

    int selectedVolumeIndex_;
    std::string currentPath_;
    bool showNewFolder_;
    bool mustExist_;
};

} // namespace os

#endif // OMNISHELL_UI_CHOOSE_FOLDER_DIALOG_HPP

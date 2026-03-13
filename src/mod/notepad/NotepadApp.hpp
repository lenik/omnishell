#ifndef OMNISHELL_APP_NOTEPAD_HPP
#define OMNISHELL_APP_NOTEPAD_HPP

#include "../../core/Module.hpp"

#include <bas/volume/VolumeFile.hpp>

#include <wx/textctrl.h>
#include <wx/wx.h>

namespace os {

/**
 * Notepad Application Module
 *
 * Basic text editor using VFS (VolumeFile) for all file I/O.
 */
class NotepadApp : public Module {
public:
    NotepadApp();
    virtual ~NotepadApp();

    virtual void run() override;

    void initializeMetadata();

private:
    void createMainWindow();
    void createMenuBar();
    void setupEvents();

    bool loadFile(const VolumeFile& vf);
    bool saveFile(const VolumeFile& vf);
    bool saveFileAs();

    void OnNew(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnSaveAs(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnFind(wxCommandEvent& event);
    void OnEncoding(wxCommandEvent& event);

    wxFrame* frame_;
    wxTextCtrl* editor_;
    wxStatusBar* statusBar_;

    VolumeFile currentFile_;
    bool modified_;
};

} // namespace os

#endif // OMNISHELL_APP_NOTEPAD_HPP

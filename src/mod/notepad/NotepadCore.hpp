#ifndef OMNISHELL_CORE_NOTEPAD_HPP
#define OMNISHELL_CORE_NOTEPAD_HPP

#include "wx/uiframe.hpp"

#include <bas/fmt/FileForm.hpp>
#include <bas/ui/arch/UIFragment.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/choicdlg.h>
#include <wx/event.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/textdlg.h>

class NotepadCore : public UIFragment, public IFileForm {
  public:
    NotepadCore(VolumeManager* volumeManager);
    virtual ~NotepadCore() {}

    void createFragmentView(CreateViewContext* ctx) override;

    wxEvtHandler* getEventHandler() override;

    /** Open a file into the editor (VFS). Safe to call after uiFrame construction. */
    bool openFile(VolumeFile file) { return uiRestoreObject(file); }

  private:
    VolumeManager* m_volumeManager;
    VolumeFile m_file;
    bool m_loaded{false};
    bool m_modified{false};

    uiFrame* m_frame;
    wxTextCtrl* m_text;
    wxString m_encoding{"UTF-8"};

    void persistObject(VolumeFile file) override;
    void restoreObject(VolumeFile file) override;

    bool uiPersistObject(VolumeFile file);
    bool uiRestoreObject(VolumeFile file);
    bool uiSaveAs();

    void onFrameClose(wxCloseEvent& event);
    void onFrameText(wxCommandEvent& event);

    void onNew(PerformContext* ctx);
    void onOpen(PerformContext*);
    void onSave(PerformContext*);
    void onSaveAs(PerformContext*);
    void onUndo(PerformContext*);
    void onRedo(PerformContext*);
    void onSelectAll(PerformContext*);
    void onClear(PerformContext*);
    void onCut(PerformContext*);
    void onCopy(PerformContext*);
    void onPaste(PerformContext*);
    void onZoomIn(PerformContext*);
    void onZoomOut(PerformContext*);
    void onZoomReset(PerformContext*);

    void onFind(PerformContext*);
    void onReplace(PerformContext*);
    void onEncoding(PerformContext*);
};

#endif // OMNISHELL_CORE_NOTEPAD_HPP
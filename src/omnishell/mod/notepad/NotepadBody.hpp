#ifndef NOTEPAD_BODY_HPP
#define NOTEPAD_BODY_HPP

#include "../../core/App.hpp"

#include <bas/fmt/FileForm.hpp>
#include <bas/ui/arch/UIFragment.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/choicdlg.h>
#include <wx/event.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/textdlg.h>

#include <optional>

namespace os {

class NotepadBody : public UIFragment, public IFileForm {
  public:
    NotepadBody(App* app);
    virtual ~NotepadBody() {}

    void createFragmentView(CreateViewContext* ctx) override;

    /** Open a file into the editor (VFS). Safe to call after uiFrame construction. */
    bool openFile(VolumeFile file) { return uiRestoreObject(file); }

  private:
    VolumeManager* volumeManager() const { return m_app ? m_app->volumeManager.get() : nullptr; }

    App* m_app;
    std::optional<VolumeFile> m_file;
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

} // namespace os

#endif // NOTEPAD_BODY_HPP
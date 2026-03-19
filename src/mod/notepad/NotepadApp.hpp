#ifndef OMNISHELL_APP_NOTEPAD_HPP
#define OMNISHELL_APP_NOTEPAD_HPP

#include "NotepadCore.hpp"

#include "../../core/Module.hpp"
#include "../../core/ModuleRegistry.hpp"

#include <bas/wx/uiframe.hpp>

namespace os {

/**
 * Notepad Application Module
 *
 * Basic text editor using VFS (VolumeFile) for all file I/O.
 */
class NotepadApp : public Module {
  public:
    NotepadApp(CreateModuleContext* ctx);
    virtual ~NotepadApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

    /** Open a file in a new Notepad window. */
    static ProcessPtr open(VolumeManager* volumeManager, VolumeFile file);

    // void createFragmentView(CreateViewContext* ctx) override;
    // wxEvtHandler* getEventHandler() override;

  private:
    VolumeManager* m_volumeManager;
    NotepadCore m_core;

    // wxTextCtrl* editor_;
    // wxStatusBar* statusBar_;

    void onAbout(PerformContext*);
};

} // namespace os

#endif // OMNISHELL_APP_NOTEPAD_HPP

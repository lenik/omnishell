#ifndef NOTEPAD_APP_HPP
#define NOTEPAD_APP_HPP

#include "../../core/Module.hpp"

#include <bas/volume/VolumeFile.hpp>

class VolumeManager;

namespace os {

class App;

/**
 * Notepad Application Module
 *
 * Basic text editor using VFS (VolumeFile) for all file I/O.
 */
class NotepadApp : public Module {
  public:
    NotepadApp(CreateModuleContext* ctx);
    virtual ~NotepadApp();

    ProcessPtr run(const RunConfig& config) override;

    void initializeMetadata();

    /** Open a file in a new Notepad window. */
    static ProcessPtr open(VolumeManager* volumeManager, VolumeFile file);

  private:
    App* m_app;
};

} // namespace os

#endif // NOTEPAD_APP_HPP

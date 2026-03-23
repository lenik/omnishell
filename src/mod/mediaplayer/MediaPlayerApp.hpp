#ifndef OMNISHELL_MOD_MEDIAPLAYER_APP_HPP
#define OMNISHELL_MOD_MEDIAPLAYER_APP_HPP

#include "../../core/Module.hpp"

class VolumeFile;
class VolumeManager;

namespace os {

class App;

class MediaPlayerApp : public Module {
  public:
    explicit MediaPlayerApp(CreateModuleContext* ctx);
    ~MediaPlayerApp() override;

    void install() override;

    void initializeMetadata();
    ProcessPtr run() override;

    /** Open a media file on a volume (VFS HTTP URL in player). */
    static ProcessPtr open(VolumeManager* vm, VolumeFile file);

  private:
    App* m_app;
};

} // namespace os

#endif

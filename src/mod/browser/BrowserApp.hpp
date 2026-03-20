#ifndef OMNISHELL_MOD_BROWSER_APP_HPP
#define OMNISHELL_MOD_BROWSER_APP_HPP

#include "../../core/Module.hpp"

class VolumeFile;
class VolumeManager;

namespace os {

class BrowserApp : public Module {
  public:
    explicit BrowserApp(CreateModuleContext* ctx);
    ~BrowserApp() override;

    void install() override;

    ProcessPtr run() override;

    /** Open a file as vol://<volumeIndex><path> in the browser. */
    static ProcessPtr open(VolumeManager* vm, VolumeFile file);

  private:
    void initializeMetadata();
};

} // namespace os

#endif

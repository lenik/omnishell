#ifndef OMNISHELL_MOD_PHOTO_VIEWER_APP_HPP
#define OMNISHELL_MOD_PHOTO_VIEWER_APP_HPP

#include "../../core/Module.hpp"

#include <bas/volume/VolumeFile.hpp>

class VolumeManager;

namespace os {

class App;

class PhotoViewerApp : public Module {
public:
    explicit PhotoViewerApp(CreateModuleContext* ctx);
    ~PhotoViewerApp() override;

    void initializeMetadata();
    void install() override;

    ProcessPtr run(const RunConfig& config) override;

    static ProcessPtr openImage(VolumeManager* volumeManager, VolumeFile file);

private:
    App* m_app{nullptr};
};

} // namespace os

#endif // OMNISHELL_MOD_PHOTO_VIEWER_APP_HPP


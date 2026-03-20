#ifndef OMNISHELL_APP_PAINT_HPP
#define OMNISHELL_APP_PAINT_HPP

#include "../../core/Module.hpp"

#include <bas/volume/VolumeFile.hpp>

class VolumeManager;

namespace os {

class App;

class PaintApp : public Module {
  public:
    explicit PaintApp(CreateModuleContext* ctx);
    virtual ~PaintApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

    /** Open an image in a new Paint window. */
    static ProcessPtr openImage(VolumeManager* volumeManager, VolumeFile file);

  private:
    App* m_app;
};

} // namespace os

#endif // OMNISHELL_APP_PAINT_HPP

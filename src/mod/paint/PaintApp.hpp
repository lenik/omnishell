#ifndef OMNISHELL_APP_PAINT_HPP
#define OMNISHELL_APP_PAINT_HPP

#include "PaintBody.hpp"

#include "../../core/Module.hpp"

#include <bas/volume/VolumeFile.hpp>

#include <wx/wx.h>

class VolumeManager;

namespace os {

class PaintApp : public Module {
public:
    explicit PaintApp(CreateModuleContext* ctx);
    virtual ~PaintApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

    /** Open an image in a new Paint window. */
    static ProcessPtr openImage(VolumeManager* volumeManager, VolumeFile file);

private:
    PaintBody m_body;
};

} // namespace os

#endif // OMNISHELL_APP_PAINT_HPP


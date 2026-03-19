#ifndef OMNISHELL_APP_EXPLORER_HPP
#define OMNISHELL_APP_EXPLORER_HPP

#include "ExplorerBody.hpp"

#include "../../core/Module.hpp"

#include <bas/volume/Volume.hpp>

#include <wx/listctrl.h>
#include <wx/wx.h>

namespace os {

/**
 * Explorer Application Module
 *
 * Simple volume browser showing directory contents of a Volume.
 */
class ExplorerApp : public Module {
public:
    explicit ExplorerApp(CreateModuleContext* ctx);
    virtual ~ExplorerApp();

    virtual ProcessPtr run() override;

    void initializeMetadata();

    /**
     * Open a directory on the given volume.
     *
     * If volume is nullptr, uses the default volume.
     */
    static ProcessPtr open(Volume* volume, const std::string& dir = "/");

private:
    static ProcessPtr openInternal(Volume* volume, const std::string& dir);
};

} // namespace os

#endif // OMNISHELL_APP_EXPLORER_HPP


#ifndef SNIPPINGTOOL_APP_HPP
#define SNIPPINGTOOL_APP_HPP

#include "../../core/Module.hpp"

class VolumeManager;

namespace os {

class App;

/**
 * Snipping Tool Application Module
 *
 * Windows-style screenshot utility.
 */
class SnippingToolApp : public Module {
  public:
    SnippingToolApp(CreateModuleContext* ctx);
    virtual ~SnippingToolApp();

    ProcessPtr run(const RunConfig& config) override;

    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif // SNIPPINGTOOL_APP_HPP

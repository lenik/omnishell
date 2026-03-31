#ifndef SOUND_RECORDER_APP_HPP
#define SOUND_RECORDER_APP_HPP

#include "../../core/Module.hpp"

class VolumeManager;

namespace os {

class App;

/**
 * Sound Recorder Application Module
 *
 * Audio recording utility.
 */
class SoundRecorderApp : public Module {
  public:
    SoundRecorderApp(CreateModuleContext* ctx);
    virtual ~SoundRecorderApp();

    ProcessPtr run(const RunConfig& config) override;

    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif // SOUND_RECORDER_APP_HPP

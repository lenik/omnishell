#ifndef TASKMANAGER_APP_HPP
#define TASKMANAGER_APP_HPP

#include "../../core/Module.hpp"

class VolumeManager;

namespace os {

class App;

/**
 * Task Manager Application Module
 *
 * View and manage running processes.
 */
class TaskManagerApp : public Module {
  public:
    TaskManagerApp(CreateModuleContext* ctx);
    virtual ~TaskManagerApp();

    ProcessPtr run(const RunConfig& config) override;

    void initializeMetadata();

  private:
    App* m_app;
};

} // namespace os

#endif // TASKMANAGER_APP_HPP

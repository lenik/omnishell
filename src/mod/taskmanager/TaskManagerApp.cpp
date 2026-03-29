#include "TaskManagerApp.hpp"

#include "TaskManagerFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../ui/ThemeStyles.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <string>

using namespace ThemeStyles;
namespace os {

namespace {
constexpr const char* kTaskManagerModuleUri = "omnishell.TaskManager";
}

OMNISHELL_REGISTER_MODULE(kTaskManagerModuleUri, TaskManagerApp)

TaskManagerApp::TaskManagerApp(CreateModuleContext* ctx) : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

TaskManagerApp::~TaskManagerApp() {}

void TaskManagerApp::initializeMetadata() {
    uri = kTaskManagerModuleUri;
    name = "taskmanager";
    label = "Task Manager";
    description = "Monitor and manage running processes";
    doc = "View system processes, CPU and memory usage, and end tasks.";
    categoryId = ID_CATEGORY_SYSTEM;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("taskmanager", "icon");
}

ProcessPtr TaskManagerApp::run(const RunConfig& config) {
    (void)config;

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new TaskManagerFrame(m_app, std::string("Task Manager"));
    frame->SetSize(wxSize(800, 600));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

#include "StopWatchApp.hpp"

#include "StopWatchFrame.hpp"

#include "../../core/ModuleRegistry.hpp"

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.StopWatch", StopWatchApp)

StopWatchApp::StopWatchApp(CreateModuleContext* ctx)
    : Module(ctx)
    , m_app(ctx->getApp()) {
    initializeMetadata();
}

StopWatchApp::~StopWatchApp() {
}

void StopWatchApp::initializeMetadata() {
    uri = "omnishell.StopWatch";
    name = "stopwatch";
    label = "StopWatch";
    description = "Simple stopwatch";
    doc = "Basic stopwatch with start/stop/reset.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("stopwatch", "icon");
}

ProcessPtr StopWatchApp::run(const RunConfig& config) {
    (void)config;
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new StopWatchFrame(m_app, "StopWatch");
    frame->SetSize(wxSize(400, 300));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

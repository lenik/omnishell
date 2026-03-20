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

    image = ImageSet(Path(slv_core_pop, "interface-essential/circle-clock.svg"));
}

ProcessPtr StopWatchApp::run() {
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new StopWatchFrame(m_app, "StopWatch");
    frame->SetSize(wxSize(300, 150));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

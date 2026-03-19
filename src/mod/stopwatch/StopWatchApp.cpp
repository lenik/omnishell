#include "StopWatchApp.hpp"

#include "../../core/ModuleRegistry.hpp"

#include <bas/wx/uiframe.hpp>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.stopwatch", StopWatchApp)

StopWatchApp::StopWatchApp(CreateModuleContext* ctx)
    : Module(ctx)
    , m_body() {
    initializeMetadata();
}

StopWatchApp::~StopWatchApp() {
}

void StopWatchApp::initializeMetadata() {
    uri = "omnishell";
    name = "stopwatch";
    label = "StopWatch";
    description = "Simple stopwatch";
    doc = "Basic stopwatch with start/stop/reset.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "circle-clock.svg"));
}

ProcessPtr StopWatchApp::run() {
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    uiFrame* frame = new uiFrame("StopWatch");
    frame->addFragment(&m_body);
    frame->createView();
    frame->SetSize(wxSize(300, 150));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os


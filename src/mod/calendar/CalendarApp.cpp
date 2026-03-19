#include "CalendarApp.hpp"

#include "../../core/ModuleRegistry.hpp"

#include <bas/wx/uiframe.hpp>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.calendar", CalendarApp)

CalendarApp::CalendarApp(CreateModuleContext* ctx)
    : Module(ctx)
    , core_() {
    initializeMetadata();
}

CalendarApp::~CalendarApp() {
}

void CalendarApp::initializeMetadata() {
    uri = "omnishell";
    name = "calendar";
    label = "Calendar";
    description = "View monthly calendar";
    doc = "Simple calendar viewer.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "blank-calendar.svg"));
}

ProcessPtr CalendarApp::run() {
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    uiFrame* frame = new uiFrame("Calendar");
    frame->addFragment(&core_);
    frame->createView();
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os


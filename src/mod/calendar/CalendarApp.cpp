#include "CalendarApp.hpp"

#include "CalendarFrame.hpp"

#include "../../core/ModuleRegistry.hpp"

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.Calendar", CalendarApp)

CalendarApp::CalendarApp(CreateModuleContext* ctx) : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

CalendarApp::~CalendarApp() {}

void CalendarApp::initializeMetadata() {
    uri = "omnishell.Calendar";
    name = "calendar";
    label = "Calendar";
    description = "View monthly calendar";
    doc = "Simple calendar viewer.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("calendar", "icon");
}

ProcessPtr CalendarApp::run(const RunConfig& config) {
    (void)config;
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new CalendarFrame(m_app, "Calendar");
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

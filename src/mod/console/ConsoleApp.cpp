#include "ConsoleApp.hpp"

#include "ConsoleFrame.hpp"

#include "../../core/ModuleRegistry.hpp"

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.Console", ConsoleApp)

ConsoleApp::ConsoleApp(CreateModuleContext* ctx)
    : Module(ctx)
    , m_app(ctx->getApp()) {
    initializeMetadata();
}

ConsoleApp::~ConsoleApp() = default;

void ConsoleApp::initializeMetadata() {
    uri = "omnishell.Console";
    name = "console";
    label = "Console";
    description = "Terminal with command shell";
    doc = "Canvas terminal (xterm-style output) and builtins.";
    categoryId = ID_CATEGORY_SYSTEM;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("console", "icon");
}

ProcessPtr ConsoleApp::run(const RunConfig& config) {
    (void)config;
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new ConsoleFrame(m_app);
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

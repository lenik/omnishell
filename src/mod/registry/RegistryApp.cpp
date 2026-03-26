#include "RegistryApp.hpp"

#include "RegistryFrame.hpp"

#include "../../core/ModuleRegistry.hpp"

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.Registry", RegistryApp)

RegistryApp::RegistryApp(CreateModuleContext* ctx)
    : Module(ctx)
    , m_app(ctx->getApp()) {
    initializeMetadata();
}

RegistryApp::~RegistryApp() {}

void RegistryApp::initializeMetadata() {
    uri = "omnishell.Registry";
    name = "registry";
    label = "Registry";
    description = "View and edit registry database";
    doc = "Viewer/editor for the file-backed registry (nodes = dirs, values = .json files).";
    categoryId = ID_CATEGORY_SYSTEM;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("registry", "icon");
}

ProcessPtr RegistryApp::run(const RunConfig& config) {
    (void)config;
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new RegistryFrame(m_app, "Registry");
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

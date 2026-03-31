#include "BackgroundSettingsApp.hpp"

#include "BackgroundSettingsFrame.hpp"

#include "../../core/ModuleRegistry.hpp"
#include "../../ui/ThemeStyles.hpp"

using namespace ThemeStyles;

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.BackgroundSettings", BackgroundSettingsApp)

BackgroundSettingsApp::BackgroundSettingsApp(CreateModuleContext* ctx) //
    : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

BackgroundSettingsApp::~BackgroundSettingsApp() {}

void BackgroundSettingsApp::initializeMetadata() {
    uri = "omnishell.BackgroundSettings";
    name = "backgroundsettings";
    label = "Background Settings";
    description = "Configure desktop background";
    doc = "UI for changing desktop background.";
    categoryId = ID_CATEGORY_SYSTEM;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("backgroundsettings", "icon");
}

ProcessPtr BackgroundSettingsApp::run(const RunConfig& config) {
    (void)config;
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new BackgroundSettingsFrame(m_app, "Background Settings");
    frame->SetSize(wxSize(520, 260));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

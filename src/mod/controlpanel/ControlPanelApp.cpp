#include "ControlPanelApp.hpp"

#include "ControlPanelFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.ControlPanel", ControlPanelApp)

ControlPanelApp::ControlPanelApp(CreateModuleContext* ctx) //
    : Module(ctx) {
    initializeMetadata();
}

ControlPanelApp::~ControlPanelApp() {
    if (m_frame) {
        m_frame->Destroy();
    }
}

void ControlPanelApp::initializeMetadata() {
    uri = "omnishell.ControlPanel";
    name = "controlpanel";
    label = "Control Panel";
    description = "System configuration and settings";
    doc = "Configure system settings, manage modules, and customize your OmniShell environment.";
    categoryId = ID_CATEGORY_SYSTEM;

    image = ImageSet(Path(slv_core_pop, "interface-essential/cog-1.svg"));
}

ProcessPtr ControlPanelApp::run() {
    if (m_frame) {
        m_frame->Raise();
        m_frame->SetFocus();
        auto p = std::make_shared<Process>();
        p->uri = uri;
        p->name = name;
        p->label = label;
        p->icon = image;
        p->addWindow(m_frame);
        return p;
    }

    m_frame = new ControlPanelFrame(&app, "Control Panel");
    m_frame->SetSize(wxSize(900, 600));
    m_frame->Centre();
    m_frame->Show(true);
    auto p = std::make_shared<Process>();
    p->uri = uri;
    p->name = name;
    p->label = label;
    p->icon = image;
    p->addWindow(m_frame);
    return p;
}

} // namespace os

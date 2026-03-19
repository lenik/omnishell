#include "RegistryApp.hpp"

#include "../../core/ModuleRegistry.hpp"

#include <bas/wx/uiframe.hpp>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.registry", RegistryApp)

RegistryApp::RegistryApp(CreateModuleContext* ctx)
    : Module(ctx)
    , m_core() {
    initializeMetadata();
}

RegistryApp::~RegistryApp() {}

void RegistryApp::initializeMetadata() {
    uri = "omnishell";
    name = "registry";
    label = "Registry";
    description = "View and edit registry database";
    doc = "Simple viewer/editor for the JSON-backed RegistryDb.";
    categoryId = ID_CATEGORY_SYSTEM;

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "filter-2.svg"));
}

ProcessPtr RegistryApp::run() {
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    uiFrame* frame = new uiFrame("Registry");
    frame->addFragment(&m_core);
    frame->createView();
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os


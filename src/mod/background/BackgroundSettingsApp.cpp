#include "BackgroundSettingsApp.hpp"

#include "../../core/ModuleRegistry.hpp"
#include <bas/wx/uiframe.hpp>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.backgroundsettings", BackgroundSettingsApp)

BackgroundSettingsApp::BackgroundSettingsApp(CreateModuleContext* ctx) //
    : Module(ctx)
    , core_() {
    initializeMetadata();
}

BackgroundSettingsApp::~BackgroundSettingsApp() {
}

void BackgroundSettingsApp::initializeMetadata() {
    uri = "omnishell";
    name = "backgroundsettings";
    label = "Background Settings";
    description = "Configure desktop background";
    doc = "UI for changing desktop background.";
    categoryId = ID_CATEGORY_SYSTEM;

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "image-blur.svg"));
}

ProcessPtr BackgroundSettingsApp::run() {
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    uiFrame* frame = new uiFrame("Background Settings");
    frame->addFragment(&core_);
    frame->createView();
    frame->SetSize(wxSize(520, 260));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os


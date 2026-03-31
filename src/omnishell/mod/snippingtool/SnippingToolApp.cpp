#include "SnippingToolApp.hpp"

#include "SnippingToolFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../ui/ThemeStyles.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <string>

using namespace ThemeStyles;
namespace os {

namespace {
constexpr const char* kSnippingToolModuleUri = "omnishell.SnippingTool";
}

OMNISHELL_REGISTER_MODULE(kSnippingToolModuleUri, SnippingToolApp)

SnippingToolApp::SnippingToolApp(CreateModuleContext* ctx) : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

SnippingToolApp::~SnippingToolApp() {}

void SnippingToolApp::initializeMetadata() {
    uri = kSnippingToolModuleUri;
    name = "snippingtool";
    label = "Snipping Tool";
    description = "Screenshot utility";
    doc = "A tool for capturing screenshots of windows, regions, or the full screen.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("snippingtool", "icon");
}

ProcessPtr SnippingToolApp::run(const RunConfig& config) {
    (void)config;

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new SnippingToolFrame(m_app, std::string("Snipping Tool"));
    frame->SetSize(wxSize(600, 500));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

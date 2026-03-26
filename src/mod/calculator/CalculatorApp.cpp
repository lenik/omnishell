#include "CalculatorApp.hpp"

#include "CalculatorFrame.hpp"

#include "../../core/App.hpp"

#include <string>
#include "../../core/ModuleRegistry.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;
namespace os {

namespace {
constexpr const char* kCalculatorModuleUri = "omnishell.Calculator";
}

OMNISHELL_REGISTER_MODULE(kCalculatorModuleUri, CalculatorApp)

CalculatorApp::CalculatorApp(CreateModuleContext* ctx) : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

CalculatorApp::~CalculatorApp() {}

void CalculatorApp::initializeMetadata() {
    uri = kCalculatorModuleUri;
    name = "calculator";
    label = "Calculator";
    description = "Standard calculator";
    doc = "A calculator for performing basic and scientific arithmetic operations.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("calculator", "icon");
}

ProcessPtr CalculatorApp::run(const RunConfig& config) {
    (void)config;

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new CalculatorFrame(m_app, std::string("Calculator"));
    frame->SetSize(wxSize(320, 420));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

#include "FiveOrMoreApp.hpp"

#include "FiveOrMoreFrame.hpp"

#include "../../core/ModuleRegistry.hpp"

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.FiveOrMore", FiveOrMoreApp)

FiveOrMoreApp::FiveOrMoreApp(CreateModuleContext* ctx) //
    : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

void FiveOrMoreApp::initializeMetadata() {
    uri = "omnishell.FiveOrMore";
    name = "fiveormore";
    label = "Five or more";
    description = "Line up five or more of the same color";
    doc = "Click a ball, then an empty cell to move (along empty paths). "
          "Five or more in a row, column, or diagonal vanish and score. "
          "After each move, new balls appear (1/2/3 by Easy/Medium/Hard). "
          "Change difficulty before the first move or start a new game. R new game.";
    categoryId = ID_CATEGORY_GAME;

    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("fiveormore", "icon");
}

ProcessPtr FiveOrMoreApp::run(const RunConfig& config) {
    (void)config;
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new FiveOrMoreFrame(m_app, "Five or more");
    frame->SetClientSize(wxSize(520, 580));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

#include "SolitaireApp.hpp"

#include "SolitaireFrame.hpp"

#include "../../core/ModuleRegistry.hpp"
#include "../../ui/ThemeStyles.hpp"

namespace os {
using namespace ThemeStyles;

OMNISHELL_REGISTER_MODULE("omnishell.Solitaire", SolitaireApp)

SolitaireApp::SolitaireApp(CreateModuleContext* ctx) : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

void SolitaireApp::initializeMetadata() {
    uri = "omnishell.Solitaire";
    name = "solitaire";
    label = "Solitaire";
    description = "Klondike solitaire";
    doc =
        "Draw from stock, build tableau (descending, alternating colors), foundations A–K by suit. "
        "R new deal.";
    categoryId = ID_CATEGORY_GAME;

    image = ImageSet(Path(slv_core_pop, "money-shopping/credit-card-1.svg"));
}

ProcessPtr SolitaireApp::run() {
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new SolitaireFrame(m_app, "Solitaire");
    frame->SetClientSize(wxSize(900, 720));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

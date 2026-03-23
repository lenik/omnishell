#include "MinesweeperApp.hpp"

#include "MinesweeperFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.Minesweeper", MinesweeperApp)

MinesweeperApp::MinesweeperApp(CreateModuleContext* ctx) //
    : Module(ctx) {
    (void)ctx;
    initializeMetadata();
}

MinesweeperApp::~MinesweeperApp() {
    if (m_frame) {
        m_frame->Destroy();
        m_frame = nullptr;
    }
}

void MinesweeperApp::initializeMetadata() {
    uri = "omnishell.Minesweeper";
    name = "minesweeper";
    label = "Minesweeper";
    description = "Classic minesweeper game";
    doc = "Simple playable minesweeper game.";
    categoryId = ID_CATEGORY_GAME;

    image = ImageSet(Path(slv_core_pop, "interface-essential/bomb.svg"));
}

ProcessPtr MinesweeperApp::run(const RunConfig& config) {
    (void)config;
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

    m_frame = new MinesweeperFrame(&app, "Minesweeper");
    m_frame->SetSize(wxSize(500, 550));
    m_frame->Centre();
    m_frame->body().resetGame();
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

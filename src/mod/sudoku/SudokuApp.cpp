#include "SudokuApp.hpp"

#include "SudokuFrame.hpp"

#include "../../core/ModuleRegistry.hpp"

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.Sudoku", SudokuApp)

SudokuApp::SudokuApp(CreateModuleContext* ctx) : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

void SudokuApp::initializeMetadata() {
    uri = "omnishell.Sudoku";
    name = "sudoku";
    label = "Sudoku";
    description = "Classic 9×9 number puzzle";
    doc = "Click a cell, type 1–9, Backspace clears. R resets. Given cells are locked.";
    categoryId = ID_CATEGORY_GAME;

    image = ImageSet(Path(slv_core_pop, "interface-essential/layout-window-8.svg"));
}

ProcessPtr SudokuApp::run() {
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new SudokuFrame(m_app, "Sudoku");
    frame->SetClientSize(wxSize(520, 580));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

#include "PaintApp.hpp"

#include "../../core/ModuleRegistry.hpp"

#include <bas/volume/VolumeFile.hpp>
#include <bas/wx/uiframe.hpp>

#include <wx/mstream.h>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.paint", PaintApp)

PaintApp::PaintApp(CreateModuleContext* ctx)
    : Module(ctx)
    , m_core() {
    initializeMetadata();
}

PaintApp::~PaintApp() {
}

void PaintApp::initializeMetadata() {
    uri = "omnishell";
    name = "paint";
    label = "Paint";
    description = "Simple drawing canvas";
    doc = "Basic paint application.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "paint-palette.svg"));
}

ProcessPtr PaintApp::run() {
    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    uiFrame* frame = new uiFrame("Paint");
    frame->addFragment(&m_core);
    frame->createView();
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

ProcessPtr PaintApp::openImage(VolumeManager* volumeManager, VolumeFile file) {
    auto proc = std::make_shared<Process>();
    proc->uri = "omnishell";
    proc->name = "paint";
    proc->label = "Paint";
    std::string dir = "streamline-vectors/core/pop/interface-essential";
    proc->icon = ImageSet(Path(dir, "paint-palette.svg"));

    auto core = std::make_shared<PaintCore>();
    uiFrame* frame = new uiFrame("Paint");
    frame->addFragment(core.get());
    frame->createView();
    frame->Centre();
    frame->Show(true);

    try {
        if (file.isNotEmpty()) {
            auto data = file.readFile();
            if (!data.empty()) {
                wxMemoryInputStream ms(data.data(), data.size());
                wxImage img(ms, wxBITMAP_TYPE_ANY);
                if (img.IsOk()) {
                    core->loadImage(img);
                }
            }
        }
    } catch (...) {
    }

    frame->Bind(wxEVT_CLOSE_WINDOW, [core](wxCloseEvent& e) {
        (void)core;
        e.Skip();
    });

    proc->addWindow(frame);
    return proc;
}

} // namespace os


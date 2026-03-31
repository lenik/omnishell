#include "PhotoViewerApp.hpp"

#include "PhotoViewerFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../core/VolUrl.hpp"
#include "../../core/registry/RegistryService.hpp"
#include "../../shell/Shell.hpp"
#include "../../ui/ThemeStyles.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/msgdlg.h>

using namespace ThemeStyles;

namespace os {

namespace {
constexpr const char* kPhotoViewerUri = "omnishell.PhotoViewer";

constexpr const char* kPhotoViewerFileTypeJson =
    R"({"openwith":{"Photo Viewer":"omnishell.PhotoViewer","Paint":"omnishell.Paint"},"icon":{"asset":"streamline-vectors/core/pop/images-photography/edit-image-photo.svg"}})";

static const char* kPhotoExtensions[] = {"jpg", "jpeg", "png", "webp", "bmp", "gif", "tif", "tiff"};

} // namespace

OMNISHELL_REGISTER_MODULE(kPhotoViewerUri, PhotoViewerApp)

PhotoViewerApp::PhotoViewerApp(CreateModuleContext* ctx) : Module(ctx), m_app(ctx->getApp()) {
    initializeMetadata();
}

PhotoViewerApp::~PhotoViewerApp() = default;

void PhotoViewerApp::initializeMetadata() {
    uri = kPhotoViewerUri;
    name = "photoviewer";
    label = "Photo Viewer";
    description = "View images from volumes";
    doc = "Simple image viewer for photos stored on volumes via the VFS.";
    categoryId = ID_CATEGORY_ACCESSORIES;
    image = (m_app ? m_app->getIconTheme() : os::app.getIconTheme())->icon("photoviewer", "icon");
}

void PhotoViewerApp::install() {
    Module::install();

    IRegistry& r = registry();
    auto ensure = [&r](const std::string& key, const char* value) {
        if (!r.has(key))
            r.set(key, std::string(value));
    };

    for (const char* ext : kPhotoExtensions) {
        ensure(std::string("OmniShell.FileTypes.extension.") + ext, kPhotoViewerFileTypeJson);
    }
    r.save();
}

ProcessPtr PhotoViewerApp::run(const RunConfig& config) {
    VolumeManager* vm = ShellApp::getInstance() ? ShellApp::getInstance()->getVolumeManager() : nullptr;
    if (!vm) {
        wxMessageBox("No volume manager", "Photo Viewer", wxOK | wxICON_ERROR);
        return nullptr;
    }

    if (!config.args.empty()) {
        VolumeFile vf(vm->getDefaultVolume(), "/");
        if (parseVolUrl(vm, config.args[0], vf))
            return openImage(vm, vf);
    }

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new PhotoViewerFrame(m_app, "Photo Viewer");
    frame->SetSize(wxSize(900, 640));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

ProcessPtr PhotoViewerApp::openImage(VolumeManager* vm, VolumeFile file) {
    if (!vm) {
        wxMessageBox("No volume manager", "Photo Viewer", wxOK | wxICON_ERROR);
        return nullptr;
    }

    auto proc = std::make_shared<Process>();
    proc->uri = kPhotoViewerUri;
    proc->name = "photoviewer";
    proc->label = "Photo Viewer";
    proc->icon = os::app.getIconTheme()->icon("photoviewer", "icon");

    auto* frame = new PhotoViewerFrame(nullptr, "Photo Viewer");
    frame->SetSize(wxSize(900, 640));
    frame->Centre();
    frame->Show(true);

    frame->body().loadVolumeFile(file);

    proc->addWindow(frame);
    return proc;
}

} // namespace os


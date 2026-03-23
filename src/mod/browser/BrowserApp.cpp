#include "BrowserApp.hpp"

#include "BrowserFrame.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../shell/Shell.hpp"

#include "../../core/registry/RegistryService.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/msgdlg.h>

#include "../../ui/ThemeStyles.hpp"
using namespace ThemeStyles;

namespace os {

namespace {
constexpr const char* kBrowserModuleUri = "omnishell.Browser";

/** JSON for web-related extensions; Browser listed first in openwith. */
constexpr const char* kWebFileTypeJson =
    R"({"openwith":{"Browser":"omnishell.Browser","Notepad":"omnishell.Notepad"},"icon":{"asset":"streamline-vectors/core/flat/computer-devices/network.svg"}})";

constexpr const char* kMimeHtmlOpenWith =
    R"({"openwith":{"Browser":"omnishell.Browser","Notepad":"omnishell.Notepad"}})";

static int volumeIndexFor(VolumeManager* vm, Volume* vol) {
    if (!vm || !vol)
        return -1;
    for (size_t i = 0; i < vm->getVolumeCount(); ++i) {
        if (vm->getVolume(i) == vol)
            return static_cast<int>(i);
    }
    return -1;
}

static std::string volUrlForFile(VolumeManager* vm, const VolumeFile& file) {
    Volume* v = file.getVolume();
    int idx = volumeIndexFor(vm, v);
    if (idx < 0)
        return {};
    std::string path = file.getPath();
    if (path.empty())
        path = "/";
    if (path[0] != '/')
        path.insert(path.begin(), '/');
    return "vol://" + std::to_string(idx) + path;
}
} // namespace

OMNISHELL_REGISTER_MODULE(kBrowserModuleUri, BrowserApp)

BrowserApp::BrowserApp(CreateModuleContext* ctx) : Module(ctx) {
    (void)ctx;
    initializeMetadata();
}

BrowserApp::~BrowserApp() = default;

void BrowserApp::initializeMetadata() {
    uri = kBrowserModuleUri;
    name = "browser";
    label = "Browser";
    description = "Web browser (HTTP/HTTPS, vol://, asset://)";
    doc = "Browse the web; VFS is served by the shell HttpDaemon (HTTP/HTTPS) and the same daemon "
          "logic is used for vol:// and asset:// inside WebView.";
    categoryId = ID_CATEGORY_ACCESSORIES;

    image = ImageSet(Path(slv_core_flat, "computer-devices/network.svg"));
}

void BrowserApp::install() {
    Module::install();
    IRegistry& r = registry();

    auto ensure = [&r](const char* key, const char* value) {
        if (!r.has(key))
            r.set(key, std::string(value));
    };

    ensure("OmniShell.FileTypes.extension.html", kWebFileTypeJson);
    ensure("OmniShell.FileTypes.extension.htm", kWebFileTypeJson);
    ensure("OmniShell.FileTypes.extension.xhtml", kWebFileTypeJson);
    ensure("OmniShell.FileTypes.mime-type.text_html", kMimeHtmlOpenWith);

    r.save();
}

ProcessPtr BrowserApp::run() {
    VolumeManager* vm =
        ShellApp::getInstance() ? ShellApp::getInstance()->getVolumeManager() : nullptr;
    if (!vm) {
        wxMessageBox("No volume manager", "Browser", wxOK | wxICON_ERROR);
        return nullptr;
    }

    auto proc = std::make_shared<Process>();
    proc->uri = uri;
    proc->name = name;
    proc->label = label;
    proc->icon = image;

    auto* frame = new BrowserFrame(vm, "Browser");
    frame->body().navigateTo(wxString("asset:///"));
    frame->SetSize(wxSize(900, 640));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

ProcessPtr BrowserApp::open(VolumeManager* vm, VolumeFile file) {
    std::string url = volUrlForFile(vm, file);
    if (url.empty()) {
        wxMessageBox("Could not resolve volume for this file.", "Browser", wxOK | wxICON_WARNING);
        return nullptr;
    }

    auto proc = std::make_shared<Process>();
    proc->uri = kBrowserModuleUri;
    proc->name = "browser";
    proc->label = "Browser";
    proc->icon = ImageSet(Path(slv_core_flat, "computer-devices/network.svg"));

    auto* frame = new BrowserFrame(vm, "Browser");
    frame->body().navigateTo(wxString::FromUTF8(url.data(), url.size()));
    frame->SetSize(wxSize(900, 640));
    frame->Centre();
    frame->Show(true);
    proc->addWindow(frame);
    return proc;
}

} // namespace os

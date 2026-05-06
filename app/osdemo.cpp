/**
 * OmniShell Desktop Environment
 *
 * Main entry point. Parses VFS command-line options (see VFS_README),
 * initializes global `os::app`, then runs the wxWidgets shell.
 */
#include <omnishell/core/App.hpp>
#include <omnishell/shell/Shell.hpp>

#include <bas/proc/env.hpp>
#include <bas/proc/stackdump.h>
#include <bas/proc/AssetsRegistry.hpp>
#include <bas/volume/LocalVolume.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>
#include <bas/volume/OverlayVolume.hpp>

#include <bas/wx/app.hpp>
#include <bas/wx/uiframe.hpp>

#include <iostream>

extern "C" void omnishell_ensure_omni_assets_registered();

#if defined(__unix__) || defined(__APPLE__)
#include <getopt.h>
#endif

int main(int argc, char** argv) {
    stackdump_install_crash_handler(&stackdump_color_schema_default);
    stackdump_set_interactive(1);

    omnishell_ensure_omni_assets_registered();
    OverlayVolume* overlay = AssetsRegistry::instance().get();
    for (const auto& layer : overlay->layers()) {
        std::cout << "Layer " << layer->getUrl() << ": " << layer->getDeviceUrl() //
                  << " [ " << layer->readDir("/")->children.size() << " ]"    //
                  << std::endl;
    }

    os::app.captureLaunchContext(argc, argv);

    os::app.volumeManager = std::make_unique<VolumeManager>();
    os::app.startFiles.clear();

    bool dump = false;

#if defined(__unix__) || defined(__APPLE__)
    static struct option longopts[] = {{"dump", no_argument, nullptr, 'D'},
                                       {"open", required_argument, nullptr, 'o'},
                                       {"local", required_argument, nullptr, 'l'},
                                       {nullptr, 0, nullptr, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "Dl:u:p:o:", longopts, nullptr)) != -1) {
        switch (opt) {
        case 'D':
            dump = true;
            break;
        case 'l':
            if (optarg && optarg[0]) {
                os::app.volumeManager->addVolume(std::make_unique<LocalVolume>(optarg));
            }
            break;
        case 'o':
            if (optarg)
                os::app.startFiles.push_back(optarg);
            break;
        default:
            break;
        }
    }
#endif

    // Optional dev testdrive + auto-discover local volumes when none were added (-l).
    os::app.addDefaultLocalVolumesIfEmpty();

    if (dump) {
        Volume* vol = os::app.volumeManager->getDefaultVolume();
        if (vol) {
            try {
                auto entries = vol->readDir("/", true);
                for (const auto& [name, e] : entries->children) {
                    if (!e)
                        continue;
                    std::cout << (e->isDirectory() ? "d " : "f ") << e->name //
                              << " " << e->size << std::endl;
                }
            } catch (const std::exception& ex) {
                std::cerr << "dump error: " << ex.what() << std::endl;
            }
        }
        return false;
    }

    os::ShellApp app("OmniShell");
    return app.main(argc, argv);
}

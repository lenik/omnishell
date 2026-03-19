/**
 * OmniShell Desktop Environment
 *
 * Main entry point. Parses VFS command-line options (see VFS_README),
 * initializes global `os::app`, then runs the wxWidgets shell.
 */

#include "core/App.hpp"
#include "shell/Shell.hpp"

#include <bas/proc/env.cpp>
#include <bas/proc/stackdump.h>
#include <bas/volume/LocalVolume.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <bas/wx/app.hpp>
#include <bas/wx/uiframe.hpp>

#include <iostream>

#if defined(__unix__) || defined(__APPLE__)
#include <getopt.h>
#endif

int main(int argc, char** argv) {
    stackdump_install_crash_handler(&stackdump_color_schema_default);
    stackdump_set_interactive(1);

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

    // Auto-discover all local volumes by default.
    if (os::app.volumeManager->getVolumeCount() == 0) {
        os::app.volumeManager->addLocalVolumes();
    }

    if (dump) {
        Volume* vol = os::app.volumeManager->getDefaultVolume();
        if (vol) {
            try {
                auto entries = vol->readDir("/", true);
                for (const auto& e : entries) {
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
/**
 * OmniShell Desktop Environment
 *
 * Main entry point. Parses VFS command-line options (see VFS_README),
 * initializes global `os::app`, then runs the wxWidgets shell.
 */

 #include "core/App.hpp"
 #include "shell/Shell.hpp"

 #include "app/controlpanel/ControlPanelApp.hpp"
 #include "app/notepad/NotepadApp.hpp"

#include <bas/volume/LocalVolume.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <iostream>

#if defined(__unix__) || defined(__APPLE__)
#include <getopt.h>
#endif

static bool initAppFromCommandLine(int argc, char* argv[]) {
    os::app.volumeManager = std::make_unique<VolumeManager>();
    os::app.startFiles.clear();

    bool dump = false;

#if defined(__unix__) || defined(__APPLE__)
    static struct option longopts[] = {
        {"dump",     no_argument,       nullptr, 'D'},
        {"open",     required_argument, nullptr, 'o'},
        {"local",    required_argument, nullptr, 'l'},
        {nullptr,    0,                 nullptr,  0}
    };

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
            if (optarg) os::app.startFiles.push_back(optarg);
            break;
        default:
            break;
        }
    }
#endif

    if (os::app.volumeManager->getVolumeCount() == 0) {
        os::app.volumeManager->addVolume(std::make_unique<LocalVolume>(getHomePath()));
    }

    if (dump) {
        Volume* vol = os::app.volumeManager->getDefaultVolume();
        if (vol) {
            try {
                auto entries = vol->readDir("/", true);
                for (const auto& e : entries) {
                    std::cout << (e->isDirectory() ? "d " : "f ") << e->name << " " << e->size << "\n";
                }
            } catch (const std::exception& ex) {
                std::cerr << "dump error: " << ex.what() << "\n";
            }
        }
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (!initAppFromCommandLine(argc, argv)) {
        return 0;
    }

    // Explicit wxWidgets lifecycle management (no wxEntry()).
    wxApp::SetInstance(new os::ShellApp());
    if (!wxEntryStart(argc, argv)) {
        return 1;
    }

    int rc = 0;
    if (wxTheApp && wxTheApp->CallOnInit()) {
        rc = wxTheApp->OnRun();
        wxTheApp->OnExit();
    } else {
        rc = 1;
    }

    wxEntryCleanup();
    return rc;
}

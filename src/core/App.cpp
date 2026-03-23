#include "App.hpp"

#include <bas/proc/env.hpp>
#include <bas/volume/LocalVolume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <filesystem>
#include <iostream>

#include <getopt.h>

#if defined(__unix__) || defined(__APPLE__)
extern char** environ;
#endif

namespace os {

App app;

App::App() : volumeManager(std::make_unique<VolumeManager>()), startFiles() {}
App::~App() {}

void App::captureLaunchContext(int argc, char** argv) {
    programSelf = (argc > 0 && argv && argv[0]) ? argv[0] : "omnishell";
    runtimeEnv.clear();
#if defined(__unix__) || defined(__APPLE__)
    if (environ) {
        for (char** e = environ; *e; ++e) {
            std::string entry = *e;
            const size_t eq = entry.find('=');
            if (eq != std::string::npos)
                runtimeEnv.emplace(entry.substr(0, eq), entry.substr(eq + 1));
        }
    }
#endif
}

RunConfig App::makeRunConfig(std::vector<std::string> args) const {
    RunConfig rc;
    rc.self = programSelf;
    rc.args = std::move(args);
    rc.env = &runtimeEnv;
    return rc;
}

void App::addDefaultLocalVolumesIfEmpty() {
    if (!volumeManager || volumeManager->getVolumeCount() != 0)
        return;
    const std::string testdrive = "/home/udisk/testdrive";
    if (std::filesystem::is_directory(testdrive))
        volumeManager->addVolume(std::make_unique<LocalVolume>(testdrive));
    volumeManager->addLocalVolumes();
}

static option longopts[] = {
    {"dump", no_argument, nullptr, 'D'},
    {"open", required_argument, nullptr, 'o'},
    {"local", required_argument, nullptr, 'l'},
    {nullptr, 0, nullptr, 0},
};

void App::parseOptions(int argc, char* argv[]) {
    int opt;
    while ((opt = getopt_long(argc, argv, "Dl:u:p:o:", longopts, nullptr)) != -1) {
        switch (opt) {
        case 'D': {
            // Dump and exit
            if (volumeManager->getVolumeCount() == 0) {
                auto local = std::make_unique<LocalVolume>(bas::getHomePath());
                volumeManager->addVolume(std::move(local));
            }
            Volume* vol = volumeManager->getDefaultVolume();
            if (vol) {
                try {
                    auto entries = vol->readDir("/", true);
                    for (const auto& e : entries) {
                        std::cout << (e->isDirectory() ? "d " : "f ") << e->name << " " << e->size
                                  << std::endl;
                    }
                } catch (const std::exception& ex) {
                    std::cerr << "dump error: " << ex.what() << std::endl;
                }
            }
            volumeManager.reset();
            exit(0);
        }
        case 'l':
            if (optarg && optarg[0]) {
                auto local = std::make_unique<LocalVolume>(optarg);
                volumeManager->addVolume(std::move(local));
            }
            break;
        case 'o':
            if (optarg)
                startFiles.push_back(optarg);
            break;
        default:
            break;
        }
    }

    addDefaultLocalVolumesIfEmpty();
}

} // namespace os

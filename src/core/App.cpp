#include "App.hpp"

#include <bas/proc/env.hpp>
#include <bas/volume/LocalVolume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <iostream>

#include <getopt.h>

namespace os {

App app;

App::App() : volumeManager(std::make_unique<VolumeManager>()), startFiles() {}
App::~App() {}

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

    // If no volumes were added, auto-discover local volumes.
    if (volumeManager->getVolumeCount() == 0) {
        volumeManager->addLocalVolumes();
    }
}

} // namespace os

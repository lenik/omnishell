#include "VFSBootstrap.hpp"

#include <bas/volume/LocalVolume.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <cstdio>
#include <cstdlib>
#include <iostream>

#if defined(__unix__) || defined(__APPLE__)
#include <getopt.h>
#endif

namespace os {

namespace {

std::unique_ptr<VolumeManager> g_volumeManager;
std::string g_openPath;

} // anonymous

bool initVFSFromCommandLine(int argc, char* argv[]) {
    g_volumeManager = std::make_unique<VolumeManager>();
    g_openPath.clear();

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
            {
                // Dump and exit
                if (g_volumeManager->getVolumeCount() == 0) {
                    auto local = std::make_unique<LocalVolume>(getHomePath());
                    g_volumeManager->addVolume(std::move(local));
                }
                Volume* vol = g_volumeManager->getDefaultVolume();
                if (vol) {
                    try {
                        auto entries = vol->readDir("/", true);
                        for (const auto& e : entries) {
                            std::cout << (e->isDirectory() ? "d " : "f ") << e->name << " " << e->size << std::endl;
                        }
                    } catch (const std::exception& ex) {
                        std::cerr << "dump error: " << ex.what() << std::endl;
                    }
                }
                g_volumeManager.reset();
                return false;
            }
        case 'l':
            if (optarg && optarg[0]) {
                auto local = std::make_unique<LocalVolume>(optarg);
                g_volumeManager->addVolume(std::move(local));
            }
            break;
        case 'o':
            if (optarg) g_openPath = optarg;
            break;
        default:
            break;
        }
    }

    // If no volumes were added, add default home volume
    if (g_volumeManager->getVolumeCount() == 0) {
        g_volumeManager->addVolume(std::make_unique<LocalVolume>(getHomePath()));
    }

    return true;
}

VolumeManager* getVolumeManager() {
    return g_volumeManager.get();
}

const std::string& getOpenPath() {
    return g_openPath;
}

void shutdownVFS() {
    g_volumeManager.reset();
    g_openPath.clear();
}

} // namespace os

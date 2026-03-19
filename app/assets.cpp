#include "proc/Assets.hpp"
#include "ui/arch/ImageSet.hpp"
#include "wx/artprov.h"
#include "wx/image.h"

#include <iostream>

int main(int argc, char** argv) {

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    ImageSet icon(wxART_NEW, dir, "cog-1.svg");
    // icon.detect();
    icon.dump(std::cout);

    std::cout << "load bitmap " << icon.getAsset()->str() << std::endl;
    wxInitAllImageHandlers();

    auto path = icon.findBestMatchAssetPath(32, 32);
    std::cout << "best match 32 path: " << *path << std::endl;

    auto bmp = icon.toBitmap(32, 32);
    if (bmp && bmp->IsOk()) {
        std::cout << "Bitmap loaded from Path: " << *path << std::endl;
    } else {
        std::cout << "Failed to convert to bitmap" << std::endl;
    }

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            std::string dir = argv[i];
            if (i != 1)
                std::cout << std::endl;
            std::cout << "Listing directory: " << dir << std::endl;
            auto files = assets_listdir(dir);
            for (const auto& file : files) {
                std::cout << file->name << " " << file->size << std::endl;
            }
        }
    } else {
        std::cout << "Assets tree:" << std::endl;
        assets_dump_tree();
    }
    return 0;
}

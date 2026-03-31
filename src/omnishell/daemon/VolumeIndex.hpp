#ifndef VOLUMEINDEX_H
#define VOLUMEINDEX_H

#include <bas/volume/VolumeManager.hpp>

#include <string>

/**
 * Handles HTML volume listing and information endpoints
 */
class VolumeIndex {
private:
    VolumeManager* m_volumeManager;
    
public:
    explicit VolumeIndex(VolumeManager* volumeManager);
    
    std::string handleGet();
    std::string generateHtml();
};

#endif // VOLUMEINDEX_H

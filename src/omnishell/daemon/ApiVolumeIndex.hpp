#ifndef APIVOLUMEINDEX_H
#define APIVOLUMEINDEX_H

#include <bas/volume/VolumeManager.hpp>

#include <string>

/**
 * Handles API volume listing and information endpoints (JSON format)
 */
class ApiVolumeIndex {
private:
    VolumeManager* m_volumeManager;
    
public:
    explicit ApiVolumeIndex(VolumeManager* volumeManager);
    
    std::string handleGet();
    std::string handleList();
    std::string handleInfo(size_t volumeIndex);
};

#endif // APIVOLUMEINDEX_H

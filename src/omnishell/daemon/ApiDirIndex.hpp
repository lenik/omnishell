#ifndef APIDIRINDEX_H
#define APIDIRINDEX_H

#include <bas/volume/VolumeManager.hpp>

#include <string>

/**
 * Handles API directory listing and navigation endpoints (JSON format)
 */
class ApiDirIndex {
private:
    VolumeManager* m_volumeManager;
    
public:
    explicit ApiDirIndex(VolumeManager* volumeManager);
    
    // pathInfo is the path after /api/volume/{index}/
    // Example: if URL is /api/volume/0/subdir/file, pathInfo is "subdir/file"
    std::string handleGet(size_t volumeIndex, const std::string& pathInfo);
    std::string handleList(size_t volumeIndex, const std::string& volumePath);
};

#endif // APIDIRINDEX_H

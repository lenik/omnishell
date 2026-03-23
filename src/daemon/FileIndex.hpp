#ifndef FILEINDEX_H
#define FILEINDEX_H

#include <bas/volume/VolumeManager.hpp>

#include <string>

/**
 * Handles file operations endpoints (read, write, delete, etc.)
 */
class FileIndex {
private:
    VolumeManager* m_volumeManager;
    
    Volume* findVolumeForPath(const std::string& path, std::string& volumePath, size_t& volumeIndex);
    
public:
    explicit FileIndex(VolumeManager* volumeManager);
    
    std::string handleGet(const std::string& path);
    std::string handlePost(const std::string& path, const std::vector<uint8_t>& data);
    std::string handleDelete(const std::string& path);
    std::string handlePut(const std::string& path, const std::vector<uint8_t>& data);
};

#endif // FILEINDEX_H

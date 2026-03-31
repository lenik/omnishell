#ifndef APIFILECONTROLLER_H
#define APIFILECONTROLLER_H

#include <bas/volume/VolumeManager.hpp>

#include <string>
#include <vector>

/**
 * Handles API file operations (JSON format)
 */
class ApiFileController {
private:
    VolumeManager* m_volumeManager;
    
public:
    explicit ApiFileController(VolumeManager* volumeManager);
    
    // pathInfo is the path after /api/volume/{index}/
    std::string handleGet(size_t volumeIndex, const std::string& pathInfo);
    std::string handlePost(size_t volumeIndex, const std::string& pathInfo, const std::vector<uint8_t>& data);
    std::string handleDelete(size_t volumeIndex, const std::string& pathInfo);
    std::string handlePut(size_t volumeIndex, const std::string& pathInfo, const std::vector<uint8_t>& data);
};

#endif // APIFILECONTROLLER_H

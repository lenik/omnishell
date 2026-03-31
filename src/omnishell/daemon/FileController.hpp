#ifndef FILECONTROLLER_H
#define FILECONTROLLER_H

#include <bas/volume/VolumeManager.hpp>

#include <string>
#include <vector>

/**
 * Handles HTML file serving
 */
class FileController {
private:
    VolumeManager* m_volumeManager;
    
public:
    explicit FileController(VolumeManager* volumeManager);
    
    // pathInfo is the path after /volume/{index}/
    std::string handleGet(size_t volumeIndex, const std::string& pathInfo);
    std::string serveFile(size_t volumeIndex, const std::string& volumePath);

    /** Same file bytes + MIME as HTTP body (for custom URL handlers). */
    bool readFilePayload(size_t volumeIndex, const std::string& pathInfo, std::vector<uint8_t>& out,
                         std::string& contentType);
};

#endif // FILECONTROLLER_H

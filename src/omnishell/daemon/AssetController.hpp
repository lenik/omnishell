#ifndef ASSETCONTROLLER_H
#define ASSETCONTROLLER_H

#include <bas/util/FileTypeDetector.hpp>

#include <cstdint>
#include <string>
#include <vector>

class Volume;

/**
 * Handles asset file serving from embedded zip
 */
class AssetController {
private:
    Volume* m_assetVolume;
    FileTypeDetector m_fileTypeDetector;
    
public:
    explicit AssetController(Volume* assetVolume);
    
    // pathInfo is the path after /asset/
    std::string handleGet(const std::string& pathInfo);
    std::string serveFile(const std::string& assetPath);
    std::string generateDirectoryListing(const std::string& assetPath, const std::string& urlPath);

    bool readFilePayload(const std::string& pathInfo, std::vector<uint8_t>& out, std::string& contentType);
};

#endif // ASSETCONTROLLER_H

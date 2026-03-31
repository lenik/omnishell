#ifndef DIRINDEX_H
#define DIRINDEX_H

#include <bas/util/FileTypeDetector.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <string>

/**
 * Handles HTML directory listing (like Apache index page)
 */
class DirIndex {
private:
    VolumeManager* m_volumeManager;
    FileTypeDetector m_fileTypeDetector;
    
public:
    explicit DirIndex(VolumeManager* volumeManager);
    
    // pathInfo is the path after /volume/{index}/
    // Example: if URL is /volume/0/subdir/file, pathInfo is "subdir/file"
    std::string handleGet(size_t volumeIndex, const std::string& pathInfo);
    std::string generateHtml(size_t volumeIndex, const std::string& volumePath, const std::string& urlPath);
};

#endif // DIRINDEX_H

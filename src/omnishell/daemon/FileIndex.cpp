#include "FileIndex.hpp"

#include <bas/volume/VolumeManager.hpp>

#include <sstream>

FileIndex::FileIndex(VolumeManager* volumeManager)
    : m_volumeManager(volumeManager) {
}

Volume* FileIndex::findVolumeForPath(const std::string& path, std::string& volumePath, size_t& volumeIndex) {
    // Path format: /api/volumes/{index}/file{/path}
    // Example: /api/volumes/0/file/subdir/file.txt
    
    if (path.find("/api/volumes/") != 0) {
        return nullptr;
    }
    
    size_t volStart = 14; // After "/api/volumes/"
    size_t volEnd = path.find('/', volStart);
    if (volEnd == std::string::npos) {
        return nullptr;
    }
    
    std::string indexStr = path.substr(volStart, volEnd - volStart);
    volumeIndex = std::stoul(indexStr);
    
    if (volumeIndex >= m_volumeManager->getVolumeCount()) {
        return nullptr;
    }
    
    // Extract file path
    size_t fileStart = path.find("/file");
    if (fileStart == std::string::npos) {
        return nullptr;
    }
    
    volumePath = path.substr(fileStart + 5); // After "/file"
    if (volumePath.empty()) {
        return nullptr;
    }
    
    return m_volumeManager->getVolume(volumeIndex);
}

std::string FileIndex::handleGet(const std::string& path) {
    std::string volumePath;
    size_t volumeIndex;
    Volume* volume = findVolumeForPath(path, volumePath, volumeIndex);
    
    if (!volume) {
        return "{\"error\":\"Invalid path or volume not found\"}";
    }
    
    try {
        if (!volume->isFile(volumePath)) {
            return "{\"error\":\"Path is not a file\"}";
        }
        
        auto fileData = volume->readFile(volumePath);
        // Return as base64 or binary - for now return metadata
        std::ostringstream json;
        json << "{\"volumeIndex\":" << volumeIndex << ","
             << "\"path\":\"" << volumePath << "\","
             << "\"size\":" << fileData.size() << "}";
        return json.str();
    } catch (...) {
        return "{\"error\":\"Failed to read file\"}";
    }
}

std::string FileIndex::handlePost(const std::string& path, const std::vector<uint8_t>& data) {
    std::string volumePath;
    size_t volumeIndex;
    Volume* volume = findVolumeForPath(path, volumePath, volumeIndex);
    
    if (!volume) {
        return "{\"error\":\"Invalid path or volume not found\"}";
    }
    
    try {
        volume->writeFile(volumePath, data);
        return "{\"success\":true}";
    } catch (...) {
        return "{\"error\":\"Failed to write file\"}";
    }
}

std::string FileIndex::handleDelete(const std::string& path) {
    std::string volumePath;
    size_t volumeIndex;
    Volume* volume = findVolumeForPath(path, volumePath, volumeIndex);
    
    if (!volume) {
        return "{\"error\":\"Invalid path or volume not found\"}";
    }
    
    try {
        if (volume->isDirectory(volumePath)) {
            volume->removeDirectory(volumePath);
        } else {
            volume->removeFile(volumePath);
        }
        return "{\"success\":true}";
    } catch (...) {
        return "{\"error\":\"Failed to delete\"}";
    }
}

std::string FileIndex::handlePut(const std::string& path, const std::vector<uint8_t>& data) {
    return handlePost(path, data);
}

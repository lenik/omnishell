#include "ApiDirIndex.hpp"

#include <bas/volume/VolumeManager.hpp>

#include <cstdlib>
#include <sstream>

ApiDirIndex::ApiDirIndex(VolumeManager* volumeManager)
    : m_volumeManager(volumeManager) {
}

std::string ApiDirIndex::handleGet(size_t volumeIndex, const std::string& pathInfo) {
    if (volumeIndex >= m_volumeManager->getVolumeCount()) {
        return "{\"error\":\"Volume index out of range\"}";
    }
    
    // Convert pathInfo to volume path (ensure it starts with /)
    std::string volumePath = pathInfo.empty() ? "/" : "/" + pathInfo;
    
    return handleList(volumeIndex, volumePath);
}

std::string ApiDirIndex::handleList(size_t volumeIndex, const std::string& volumePath) {
    auto volume = m_volumeManager->getVolume(volumeIndex);
    if (!volume) {
        return "{\"error\":\"Volume not found\"}";
    }
    
    try {
        if (!volume->isDirectory(volumePath)) {
            return "{\"error\":\"Path is not a directory\"}";
        }
        
        auto dir = volume->readDir(volumePath);
        std::ostringstream json;
        json << "{\"volumeIndex\":" << volumeIndex << ","
             << "\"path\":\"" << volumePath << "\","
             << "\"entries\":[";
        
        size_t i = 0;
        for (const auto& [name, child] : dir->children) {
            if (i > 0) json << ",";
            // Skip . and ..
            if (name == "." || name == "..") {
                continue;
            }
            std::string timeStr = "-";
            if (child->epochNano > 0) {
                auto time = child->modifiedTime();
                timeStr = std::to_string(child->epochSeconds());
            }
            json << "{\"name\":\"" << name << "\","
                 << "\"isDirectory\":" << (child->isDirectory() ? "true" : "false") << ","
                 << "\"size\":" << child->size << ","
                 << "\"modifiedTime\":" << timeStr << "}";
        }
        
        json << "]}";
        return json.str();
    } catch (...) {
        return "{\"error\":\"Failed to read directory\"}";
    }
}

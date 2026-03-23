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
        
        auto entries = volume->readDir(volumePath);
        std::ostringstream json;
        json << "{\"volumeIndex\":" << volumeIndex << ","
             << "\"path\":\"" << volumePath << "\","
             << "\"entries\":[";
        
        for (size_t i = 0; i < entries.size(); ++i) {
            if (i > 0) json << ",";
            const auto& st = entries[i];
            // Skip . and ..
            if (st->name == "." || st->name == "..") {
                continue;
            }
            json << "{\"name\":\"" << st->name << "\","
                 << "\"isDirectory\":" << (st->isDirectory() ? "true" : "false") << ","
                 << "\"size\":" << st->size << ","
                 << "\"modifiedTime\":" << st->modifiedTime << "}";
        }
        
        json << "]}";
        return json.str();
    } catch (...) {
        return "{\"error\":\"Failed to read directory\"}";
    }
}

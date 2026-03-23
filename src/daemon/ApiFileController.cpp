#include "ApiFileController.hpp"

#include <bas/volume/VolumeManager.hpp>

#include <sstream>

ApiFileController::ApiFileController(VolumeManager* volumeManager)
    : m_volumeManager(volumeManager) {
}

std::string ApiFileController::handleGet(size_t volumeIndex, const std::string& pathInfo) {
    if (volumeIndex >= m_volumeManager->getVolumeCount()) {
        return "{\"error\":\"Volume index out of range\"}";
    }
    
    // Convert pathInfo to volume path (ensure it starts with /)
    std::string volumePath = pathInfo.empty() ? "/" : "/" + pathInfo;
    
    auto volume = m_volumeManager->getVolume(volumeIndex);
    if (!volume->isFile(volumePath)) {
        return "{\"error\":\"File not found\"}";
    }
    
    try {
        auto fileData = volume->readFile(volumePath);
        std::ostringstream json;
        json << "{\"volumeIndex\":" << volumeIndex << ","
             << "\"path\":\"" << volumePath << "\","
             << "\"size\":" << fileData.size() << "}";
        return json.str();
    } catch (...) {
        return "{\"error\":\"Failed to read file\"}";
    }
}

std::string ApiFileController::handlePost(size_t volumeIndex, const std::string& pathInfo, const std::vector<uint8_t>& data) {
    if (volumeIndex >= m_volumeManager->getVolumeCount()) {
        return "{\"error\":\"Volume index out of range\"}";
    }
    
    std::string volumePath = pathInfo.empty() ? "/" : "/" + pathInfo;
    auto volume = m_volumeManager->getVolume(volumeIndex);
    
    try {
        volume->writeFile(volumePath, data);
        return "{\"success\":true}";
    } catch (...) {
        return "{\"error\":\"Failed to write file\"}";
    }
}

std::string ApiFileController::handleDelete(size_t volumeIndex, const std::string& pathInfo) {
    if (volumeIndex >= m_volumeManager->getVolumeCount()) {
        return "{\"error\":\"Volume index out of range\"}";
    }
    
    std::string volumePath = pathInfo.empty() ? "/" : "/" + pathInfo;
    auto volume = m_volumeManager->getVolume(volumeIndex);
    
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

std::string ApiFileController::handlePut(size_t volumeIndex, const std::string& pathInfo, const std::vector<uint8_t>& data) {
    return handlePost(volumeIndex, pathInfo, data);
}

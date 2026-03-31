#include "ApiVolumeIndex.hpp"

#include <bas/volume/VolumeManager.hpp>

#include <cstdlib>
#include <sstream>

ApiVolumeIndex::ApiVolumeIndex(VolumeManager* volumeManager)
    : m_volumeManager(volumeManager) {
}

std::string ApiVolumeIndex::handleGet() {
    return handleList();
}

std::string ApiVolumeIndex::handleList() {
    std::ostringstream json;
    json << "{\"volumes\":[";
    
    for (size_t i = 0; i < m_volumeManager->getVolumeCount(); ++i) {
        if (i > 0) json << ",";
        auto volume = m_volumeManager->getVolume(i);
        json << "{\"index\":" << i << ","
             << "\"class\":\"" << volume->getClass() << "\","
             << "\"type\":\"" << volume->getTypeString() << "\","
             << "\"uuid\":\"" << volume->getUUID() << "\","
             << "\"label\":\"" << volume->getLabel() << "\","
             << "\"serial\":\"" << volume->getSerial() << "\"}";
    }
    
    json << "]}";
    return json.str();
}

std::string ApiVolumeIndex::handleInfo(size_t volumeIndex) {
    if (volumeIndex >= m_volumeManager->getVolumeCount()) {
        return "{\"error\":\"Volume index out of range\"}";
    }
    
    auto volume = m_volumeManager->getVolume(volumeIndex);
    std::ostringstream json;
    json << "{\"index\":" << volumeIndex << ","
         << "\"class\":\"" << volume->getClass() << "\","
         << "\"type\":\"" << volume->getTypeString() << "\","
         << "\"uuid\":\"" << volume->getUUID() << "\","
         << "\"label\":\"" << volume->getLabel() << "\","
         << "\"serial\":\"" << volume->getSerial() << "\"}";
    return json.str();
}

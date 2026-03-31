#include "FileController.hpp"

#include <bas/util/FileTypeDetector.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <sstream>

FileController::FileController(VolumeManager* volumeManager)
    : m_volumeManager(volumeManager) {
    if (!volumeManager) throw std::invalid_argument("FileController::FileController: null volumeManager");
}

std::string FileController::handleGet(size_t volumeIndex, const std::string& pathInfo) {
    if (!m_volumeManager) throw std::invalid_argument("FileController::handleGet: null volumeManager");
    if (volumeIndex >= m_volumeManager->getVolumeCount()) {
        return "HTTP/1.1 404 Not Found\r\n\r\n";
    }
    
    // Convert pathInfo to volume path (ensure it starts with /)
    std::string volumePath = pathInfo.empty() ? "/" : "/" + pathInfo;
    
    auto volume = m_volumeManager->getVolume(volumeIndex);
    if (!volume->isFile(volumePath)) {
        return "HTTP/1.1 404 Not Found\r\n\r\n";
    }
    
    return serveFile(volumeIndex, volumePath);
}

std::string FileController::serveFile(size_t volumeIndex, const std::string& volumePath) {
    if (!m_volumeManager) throw std::invalid_argument("FileController::serveFile: null volumeManager");
    if (volumePath.empty()) throw std::invalid_argument("FileController::serveFile: volumePath is empty");
    auto volume = m_volumeManager->getVolume(volumeIndex);
    auto fileData = volume->readFile(volumePath);
    
    // Extract filename for MIME type detection
    std::string filename;
    size_t lastSlash = volumePath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        filename = volumePath.substr(lastSlash + 1);
    } else {
        filename = volumePath;
    }
    
    // Use FileTypeDetector to get content type
    std::string contentType = FileTypeDetector::getMimeType(filename);
    
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Content-Length: " << fileData.size() << "\r\n"
             << "\r\n";
    
    // Note: In a real implementation, we'd need to send the binary data separately
    // For now, this is a simplified version
    response.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
    
    return response.str();
}

bool FileController::readFilePayload(size_t volumeIndex, const std::string& pathInfo, std::vector<uint8_t>& out,
                                   std::string& contentType) {
    if (!m_volumeManager)
        return false;
    if (volumeIndex >= m_volumeManager->getVolumeCount())
        return false;
    std::string volumePath = pathInfo.empty() ? "/" : "/" + pathInfo;
    auto volume = m_volumeManager->getVolume(volumeIndex);
    if (!volume->isFile(volumePath))
        return false;
    out = volume->readFile(volumePath);
    std::string filename;
    size_t lastSlash = volumePath.find_last_of('/');
    filename = lastSlash != std::string::npos ? volumePath.substr(lastSlash + 1) : volumePath;
    contentType = FileTypeDetector::getMimeType(filename);
    return true;
}

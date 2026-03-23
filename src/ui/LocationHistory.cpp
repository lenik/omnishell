#include "LocationHistory.hpp"

#include <bas/volume/Volume.hpp>

#include <cassert>

LocationHistory::LocationHistory(const Volume* volume, std::string_view path)
    : m_historyIndex(0) {
    if (!volume) throw std::invalid_argument("LocationHistory::LocationHistory: null volume");
    m_history.push_back({ volume, path });
}

void LocationHistory::go(const Volume* volume, std::string_view path) {
    if (!volume) throw std::invalid_argument("LocationHistory::go: null volume");
    if (!volume->exists(normalizePath(path))) {
        return;
    }
    
    const Volume* currentVolume = m_history[m_historyIndex].volume;
    std::string currentPath = m_history[m_historyIndex].path;
    if (currentVolume == volume && currentPath == path) return;

    addToHistory(volume, path);
    notify();
}

void LocationHistory::goBack() {
    if (!canGoBack()) return;
    m_historyIndex--;
    notify();
}

void LocationHistory::goForward() {
    if (!canGoForward()) return;
    m_historyIndex++;
    notify();
}

void LocationHistory::goUp() {
    if (!canGoUp()) return;
    
    const Volume* currentVolume = m_history[m_historyIndex].volume;
    std::string currentPath = m_history[m_historyIndex].path;

    std::string parent = getParentPath(currentPath);
    if (parent != currentPath)
        go(currentVolume, parent);
}

void LocationHistory::goHome() {
    go(m_history[0].volume, m_history[0].path);
}

void LocationHistory::notify() {
    for (auto& listener : m_locationChangeListeners) {
        listener(m_history[m_historyIndex]);
    }
}

bool LocationHistory::canGoBack() const {
    return m_historyIndex > 0;
}

bool LocationHistory::canGoForward() const {
    return m_historyIndex < m_history.size() - 1;
}

bool LocationHistory::canGoUp() const {
    std::string currentPath = m_history[m_historyIndex].path;
    std::string parent = getParentPath(currentPath);
    return parent != currentPath;
}

void LocationHistory::onLocationChange(std::function<void(const Location& location)> callback) {
    m_locationChangeListeners.push_back(callback);
}

void LocationHistory::addToHistory(const Volume* volume, std::string_view path) {
    if (!volume) throw std::invalid_argument("LocationHistory::addToHistory: null volume");
    if (m_historyIndex < m_history.size() - 1)
        m_history.erase(m_history.begin() + m_historyIndex + 1, m_history.end());
    Location loc{ volume, path };
    if (m_history.empty() || m_history.back().volume != loc.volume || m_history.back().path != loc.path) {
        m_history.push_back(loc);
        m_historyIndex = m_history.size() - 1;
    }
    const size_t maxSize = 100;
    if (m_history.size() > maxSize) {
        m_history.erase(m_history.begin());
        m_historyIndex--;
    }
}

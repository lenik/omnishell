#include "Location.hpp"

#include <bas/volume/Volume.hpp>

#include <algorithm>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>

std::string normalizePath(std::string_view path) {
    if (path.empty()) return "/";
    std::string n(path);
    std::replace(n.begin(), n.end(), '\\', '/');
    if (n[0] != '/') n = "/" + n;
    if (n.length() > 1 && n.back() == '/') n.pop_back();
    return n;
}

std::string getParentPath(std::string_view path) {
    std::string n = normalizePath(path);
    if (n == "/") return "/";
    size_t last = n.find_last_of('/');
    if (last == 0) return "/";
    return n.substr(0, last);
}

Location::Location(Volume* volume, std::string_view path) : volume(volume), path(path) {
    if (volume == nullptr)
        throw std::invalid_argument("Location: null volume");
    if (path.empty())
        throw std::invalid_argument("Location: empty path");
}

Location::Location(const Volume* volume, std::string_view path) : volume(const_cast<Volume*>(volume)), path(path) {
    if (volume == nullptr)
        throw std::invalid_argument("Location: null volume");
    if (path.empty())
        throw std::invalid_argument("Location: empty path");
}

std::unique_ptr<VolumeFile> Location::toFile() const {
    return std::make_unique<VolumeFile>(volume, path);
}

std::unique_ptr<VolumeFile> Location::join(std::string_view path) const {
    return toFile()->resolve(path);
}

std::string Location::toString() const {
    return volume->getClassId() + ":" + path;
}

bool Location::operator ==(const Location& other) const {
    return volume == other.volume && path == other.path;
}

bool Location::operator !=(const Location& other) const {
    return volume != other.volume || path != other.path;
}

bool Location::operator <(const Location& other) const {
    if (volume != other.volume) {
        return volume->getClassId() < other.volume->getClassId();
    }
    return path < other.path;
}

bool Location::operator >(const Location& other) const {
    if (volume != other.volume) {
        return volume->getClassId() > other.volume->getClassId();
    }
    return path > other.path;
}

bool Location::operator <=(const Location& other) const {
    if (volume != other.volume) {
        return volume->getClassId() <= other.volume->getClassId();
    }
    return path <= other.path;
}

bool Location::operator >=(const Location& other) const {
    if (volume != other.volume) {
        return volume->getClassId() >= other.volume->getClassId();
    }
    return path >= other.path;
}

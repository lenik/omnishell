#ifndef LOCATION_H
#define LOCATION_H

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>

#include <memory>
#include <string>

std::string normalizePath(std::string_view path);
std::string getParentPath(std::string_view path);

struct Location {
    Volume* volume;
    std::string path;

    Location(Volume* volume, std::string_view path);
    Location(const Volume* volume, std::string_view path);

    std::unique_ptr<VolumeFile> toFile() const;
    std::unique_ptr<VolumeFile> join(std::string_view path) const;

    std::string toString() const;

    bool operator ==(const Location& other) const;
    bool operator !=(const Location& other) const;

    bool operator <(const Location& other) const;
    bool operator >(const Location& other) const;
    bool operator <=(const Location& other) const;
    bool operator >=(const Location& other) const;
};

#endif // LOCATION_H
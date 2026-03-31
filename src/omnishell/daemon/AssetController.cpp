#include "AssetController.hpp"

#include <bas/util/FileTypeDetector.hpp>
#include <bas/volume/Volume.hpp>

#include <cstring>
#include <ctime>
#include <sstream>

#define PREVIEW_IMAGE_SIZE 16384

AssetController::AssetController(Volume* assetVolume)
    : m_assetVolume(assetVolume) {
}

std::string AssetController::handleGet(const std::string& pathInfo) {
    if (!m_assetVolume) {
        return "HTTP/1.1 404 Not Found\r\n\r\n";
    }
    
    // Convert pathInfo to asset path (ensure it starts with /)
    std::string assetPath = pathInfo.empty() ? "/" : "/" + pathInfo;
    
    if (m_assetVolume->isFile(assetPath)) {
        return serveFile(assetPath);
    } else if (m_assetVolume->isDirectory(assetPath)) {
        std::string htmlStr = generateDirectoryListing(assetPath, pathInfo);
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: text/html\r\n"
                 << "Content-Length: " << htmlStr.length() << "\r\n"
                 << "\r\n"
                 << htmlStr;
        return response.str();
    } else {
        return "HTTP/1.1 404 Not Found\r\n\r\n";
    }
}

std::string AssetController::serveFile(const std::string& assetPath) {
    auto fileData = m_assetVolume->readFile(assetPath);
    
    // Extract filename for MIME type detection
    std::string filename;
    size_t lastSlash = assetPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        filename = assetPath.substr(lastSlash + 1);
    } else {
        filename = assetPath;
    }
    
    // Use FileTypeDetector to get content type
    std::string contentType = FileTypeDetector::getMimeType(filename);
    
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Content-Length: " << fileData.size() << "\r\n"
             << "\r\n";
    
    response.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
    
    return response.str();
}

std::string AssetController::generateDirectoryListing(const std::string& assetPath, const std::string& urlPath) {
    auto entries = m_assetVolume->readDir(assetPath);
    
    std::string displayPath = urlPath.empty() ? "/asset/" : "/asset/" + urlPath;
    if (displayPath.back() != '/') {
        displayPath += "/";
    }
    
    // Build breadcrumb: Assets / segment1 / segment2 / ...
    std::ostringstream breadcrumb;
    breadcrumb << "<a href=\"/asset/\">Assets</a>";
    std::string path = displayPath;
    std::string prefix = "/asset/";
    if (path.size() > prefix.size() && path.find(prefix) == 0) {
        path = path.substr(prefix.length());
    } else {
        path = "";
    }
    if (!path.empty() && path != "/") {
        std::string currentPath = prefix;
        std::istringstream pathStream(path);
        std::string segment;
        while (std::getline(pathStream, segment, '/')) {
            if (!segment.empty()) {
                currentPath += segment + "/";
                breadcrumb << " / <a href=\"" << currentPath << "\">" << segment << "</a>";
            }
        }
    }
    
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html><head>\n"
         << "<meta charset=\"utf-8\"/>\n"
         << "<meta http-equiv=\"Cache-Control\" content=\"no-cache\"/>\n"
         << "<title>Index of " << displayPath << "</title>\n"
         << "<style type=\"text/css\">\n"
         << "body { font-family: \"DejaVu Sans\", \"Liberation Sans\", Arial, sans-serif; background-color: #fff; color: #000; margin: 20px; }\n"
         << "h1 { font-size: 1.5em; margin: 0 0 10px 0; }\n"
         << ".breadcrumb { font-size: 0.9em; color: #666; margin-bottom: 10px; }\n"
         << ".breadcrumb a { color: #0066cc; text-decoration: none; }\n"
         << ".breadcrumb a:hover { text-decoration: underline; }\n"
         << "hr { border: 0; border-top: 1px solid #ccc; margin: 20px 0; }\n"
         << "table.diridx { border-collapse: collapse; table-layout: fixed; width: 100%; }\n"
         << "th { text-align: left; padding: 8px; background-color: #f0f0f0; border-bottom: 2px solid #ccc; font-weight: bold; }\n"
         << "td { padding: 8px; border-bottom: 1px solid #ddd; }\n"
         << "th.icon-col, td.icon-cell { width: 36px; min-width: 36px; max-width: 36px; padding: 6px 4px; box-sizing: border-box; vertical-align: middle; text-align: center; }\n"
         << "td.icon-cell .icon, th.icon-col .icon { width: 20px; height: 20px; max-width: 20px; max-height: 20px; object-fit: contain; display: block; margin: 0 auto; }\n"
         << "th:nth-child(2), td:nth-child(2) { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }\n"
         << "tr:hover { background-color: #f9f9f9; }\n"
         << "a { color: #0066cc; text-decoration: none; }\n"
         << "a:hover { text-decoration: underline; }\n"
         << ".name { text-align: left; }\n"
         << ".size { text-align: right; }\n"
         << ".date { text-align: left; }\n"
         << "</style>\n"
         << "</head><body>\n"
         << "<h1>Index of " << displayPath << "</h1>\n"
         << "<div class=\"breadcrumb\">" << breadcrumb.str() << "</div>\n"
         << "<hr>\n"
         << "<table class=\"diridx\">\n"
         << "<colgroup><col style=\"width:36px\" /><col /><col /><col style=\"width:9em\" /><col /></colgroup>\n"
         << "<thead><tr>"
         << "<th class=\"icon-col\"></th>"
         << "<th class=\"name\">Name</th>"
         << "<th class=\"date\">Last modified</th>"
         << "<th class=\"size\">Size</th>"
         << "<th class=\"date\">Description</th>"
         << "</tr></thead>\n"
         << "<tbody>\n";
    
    // Parent directory link
    if (assetPath != "/") {
        std::string parentPath = urlPath;
        size_t lastSlash = parentPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            parentPath = parentPath.substr(0, lastSlash);
            if (parentPath.empty()) {
                parentPath = "/asset/";
            } else {
                parentPath = "/asset/" + parentPath;
            }
        } else {
            parentPath = "/asset/";
        }
        html << "<tr>"
             << "<td class=\"icon-cell\"><img src=\"" << m_fileTypeDetector.getFolderColorIcon() << "\" class=\"icon\" alt=\"[DIR]\"></td>"
             << "<td class=\"name\"><a href=\"" << parentPath << "\">Parent Directory</a></td>"
             << "<td class=\"date\">-</td>"
             << "<td class=\"size\">-</td>"
             << "<td class=\"date\">-</td>"
             << "</tr>\n";
    }
    
    for (const auto& entry : entries) {
        // Skip . and ..
        if (entry->name == "." || entry->name == "..") {
            continue;
        }
        
        std::string entryUrl = displayPath;
        if (entryUrl.back() != '/') {
            entryUrl += "/";
        }
        entryUrl += entry->name;
        
        // Format size with human-readable units
        std::string sizeStr;
        if (entry->isDirectory()) {
            sizeStr = "-";
        } else {
            size_t size = entry->size;
            if (size < 1024) {
                sizeStr = std::to_string(size);
            } else if (size < 1024 * 1024) {
                sizeStr = std::to_string(size / 1024) + "K";
            } else if (size < 1024 * 1024 * 1024) {
                sizeStr = std::to_string(size / (1024 * 1024)) + "M";
            } else {
                sizeStr = std::to_string(size / (1024 * 1024 * 1024)) + "G";
            }
        }
        
        // Format time (Apache style: DD-MMM-YYYY HH:MM)
        std::string timeStr = "-";
        if (entry->modifiedTime > 0) {
            std::time_t time = entry->modifiedTime;
            std::tm* tm = std::localtime(&time);
            char timeBuf[64];
            std::strftime(timeBuf, sizeof(timeBuf), "%d-%b-%Y %H:%M", tm);
            timeStr = timeBuf;
        }
        
        std::string displayName = entry->name;
        if (entry->isDirectory()) {
            displayName += "/";
        }
        
        // Get icon path
        std::string iconPath = m_fileTypeDetector.getColorIcon(entry->name);
        if (entry->isDirectory()) {
            iconPath = m_fileTypeDetector.getFolderColorIcon();
        } else {
            // For small image files (<4K), use the image itself as icon
            if (FileTypeDetector::isImageFile(entry->name) && entry->size > 0 && entry->size < PREVIEW_IMAGE_SIZE) {
                iconPath = entryUrl;
            }
        }
        
        html << "<tr>"
             << "<td class=\"icon-cell\"><img src=\"" << iconPath << "\" class=\"icon\" alt=\"\"></td>"
             << "<td class=\"name\"><a href=\"" << entryUrl << "\">" << displayName << "</a></td>"
             << "<td class=\"date\">" << timeStr << "</td>"
             << "<td class=\"size\">" << sizeStr << "</td>"
             << "<td class=\"date\">-</td>"
             << "</tr>\n";
    }
    
    html << "</tbody>\n"
         << "</table>\n"
         << "<hr>\n"
         << "</body></html>\n";

    return html.str();
}

bool AssetController::readFilePayload(const std::string& pathInfo, std::vector<uint8_t>& out,
                                      std::string& contentType) {
    if (!m_assetVolume)
        return false;
    std::string assetPath = pathInfo.empty() ? "/" : "/" + pathInfo;
    if (!m_assetVolume->isFile(assetPath))
        return false;
    out = m_assetVolume->readFile(assetPath);
    std::string filename;
    size_t lastSlash = assetPath.find_last_of('/');
    filename = lastSlash != std::string::npos ? assetPath.substr(lastSlash + 1) : assetPath;
    contentType = FileTypeDetector::getMimeType(filename);
    return true;
}

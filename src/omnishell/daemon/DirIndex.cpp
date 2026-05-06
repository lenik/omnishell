#include "DirIndex.hpp"

#include <bas/util/FileTypeDetector.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string_view>

#define PREVIEW_IMAGE_SIZE 16384

namespace {

std::string toLowerAscii(std::string s) {
    for (auto& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string htmlAttrEscape(std::string_view s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '&':
            o += "&amp;";
            break;
        case '"':
            o += "&quot;";
            break;
        case '<':
            o += "&lt;";
            break;
        default:
            o += c;
            break;
        }
    }
    return o;
}

} // namespace

DirIndex::DirIndex(VolumeManager* volumeManager) : m_volumeManager(volumeManager) {}

std::string DirIndex::handleGet(size_t volumeIndex, const std::string& pathInfo) {
    if (volumeIndex >= m_volumeManager->getVolumeCount()) {
        return "<html><body><h1>404 - Volume not found</h1></body></html>";
    }

    // Convert pathInfo to volume path (ensure it starts with /)
    std::string volumePath = pathInfo.empty() ? "/" : "/" + pathInfo;

    auto volume = m_volumeManager->getVolume(volumeIndex);
    if (!volume->isDirectory(volumePath)) {
        return "<html><body><h1>404 - Not a directory</h1></body></html>";
    }

    return generateHtml(volumeIndex, volumePath,
                        "/volume/" + std::to_string(volumeIndex) + "/" + pathInfo);
}

std::string DirIndex::generateHtml(size_t volumeIndex, const std::string& volumePath,
                                   const std::string& urlPath) {
    auto volume = m_volumeManager->getVolume(volumeIndex);
    auto dir = volume->readDir(volumePath);
    std::vector<DirNode*> sortedChildren;
    sortedChildren.reserve(dir->children.size());
    for (const auto& child : dir->children) {
        sortedChildren.push_back(child.second.get());
    }
    std::sort(sortedChildren.begin(), sortedChildren.end(), [](const auto* a, const auto* b) {
        const std::string ka = a->name + (a->isDirectory() ? "/" : "");
        const std::string kb = b->name + (b->isDirectory() ? "/" : "");
        return toLowerAscii(ka) < toLowerAscii(kb);
    });

    // Generate breadcrumb navigation
    std::ostringstream breadcrumb;
    breadcrumb << "<a href=\"/volume/\">Volumes</a>";
    breadcrumb << " / <a href=\"/volume/" << volumeIndex << "/\">Volume " << volumeIndex << "</a>";

    // Parse path segments from urlPath
    std::string path = urlPath;
    std::string prefix = "/volume/" + std::to_string(volumeIndex) + "/";

    // Remove the volume prefix to get the relative path
    if (path.find(prefix) == 0) {
        path = path.substr(prefix.length());
    } else if (path == "/volume/" + std::to_string(volumeIndex)) {
        path = "";
    }

    // Build breadcrumb from path segments
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
         << "<title>Index of " << urlPath << "</title>\n"
         << "<style type=\"text/css\">\n"
         << "body { font-family: \"DejaVu Sans\", \"Liberation Sans\", Arial, sans-serif; "
            "background-color: #fff; color: #000; margin: 20px; }\n"
         << "h1 { font-size: 1.5em; margin: 0 0 10px 0; }\n"
         << ".breadcrumb { font-size: 0.9em; color: #666; margin-bottom: 10px; }\n"
         << ".breadcrumb a { color: #0066cc; text-decoration: none; }\n"
         << ".breadcrumb a:hover { text-decoration: underline; }\n"
         << "hr { border: 0; border-top: 1px solid #ccc; margin: 20px 0; }\n"
         << "table.diridx { border-collapse: collapse; table-layout: fixed; width: 100%; }\n"
         << "th { text-align: left; padding: 8px; background-color: #f0f0f0; border-bottom: 2px "
            "solid #ccc; font-weight: bold; }\n"
         << "td { padding: 8px; border-bottom: 1px solid #ddd; }\n"
         << "th.icon-col, td.icon-cell { width: 36px; min-width: 36px; max-width: 36px; padding: "
            "6px 4px; box-sizing: border-box; vertical-align: middle; text-align: center; }\n"
         << "td.icon-cell .icon, th.icon-col .icon { width: 20px; height: 20px; max-width: 20px; "
            "max-height: 20px; object-fit: contain; display: block; margin: 0 auto; }\n"
         << "th:nth-child(2), td:nth-child(2) { overflow: hidden; text-overflow: ellipsis; "
            "white-space: nowrap; }\n"
         << "th.diridx-th { cursor: pointer; user-select: none; }\n"
         << "th.diridx-th:hover { background-color: #e8e8e8; }\n"
         << "th.icon-col { cursor: default; }\n"
         << "th.icon-col:hover { background-color: #f0f0f0; }\n"
         << ".sort-ind { font-size: 0.75em; margin-left: 4px; opacity: 0.75; }\n"
         << "tr:hover { background-color: #f9f9f9; }\n"
         << "a { color: #0066cc; text-decoration: none; }\n"
         << "a:hover { text-decoration: underline; }\n"
         << ".name { text-align: left; }\n"
         << ".size { text-align: right; }\n"
         << ".date { text-align: left; }\n"
         << "</style>\n"
         << "</head><body>\n"
         << "<h1>Index of " << urlPath << "</h1>\n"
         << "<div class=\"breadcrumb\">" << breadcrumb.str() << "</div>\n"
         << "<hr>\n"
         << "<table class=\"diridx\">\n"
         << "<colgroup><col style=\"width:36px\" /><col /><col /><col style=\"width:9em\" /><col "
            "/></colgroup>\n"
         << "<thead><tr>"
         << "<th class=\"icon-col\"></th>"
         << "<th class=\"name diridx-th\" data-sort-key=\"name\" tabindex=\"0\" "
            "scope=\"col\">Name<span class=\"sort-ind\" aria-hidden=\"true\"></span></th>"
         << "<th class=\"date diridx-th\" data-sort-key=\"mtime\" tabindex=\"0\" "
            "scope=\"col\">Last modified<span class=\"sort-ind\" aria-hidden=\"true\"></span></th>"
         << "<th class=\"size diridx-th\" data-sort-key=\"size\" tabindex=\"0\" "
            "scope=\"col\">Size<span class=\"sort-ind\" aria-hidden=\"true\"></span></th>"
         << "<th class=\"date diridx-th\" data-sort-key=\"desc\" tabindex=\"0\" "
            "scope=\"col\">Description<span class=\"sort-ind\" aria-hidden=\"true\"></span></th>"
         << "</tr></thead>\n"
         << "<tbody id=\"diridx-body\">\n";

    // Parent directory link
    if (volumePath != "/") {
        std::string parentPath = urlPath;
        size_t lastSlash = parentPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            parentPath = parentPath.substr(0, lastSlash);
            if (parentPath.empty())
                parentPath = "/volume/" + std::to_string(volumeIndex) + "/";
        }
        html << "<tr class=\"diridx-parent\">"
             << "<td class=\"icon-cell\"><img src=\"" << m_fileTypeDetector.getFolderColorIcon()
             << "\" class=\"icon\" alt=\"[DIR]\"></td>"
             << "<td class=\"name\"><a href=\"" << parentPath << "\">Parent Directory</a></td>"
             << "<td class=\"date\">-</td>"
             << "<td class=\"size\">-</td>"
             << "<td class=\"date\">-</td>"
             << "</tr>\n";
    }

    for (const auto* child : sortedChildren) {
        const auto& name = child->name;
        // Skip . and ..
        if (name == "." || name == "..") {
            continue;
        }

        std::string entryUrl = urlPath;
        if (!entryUrl.empty() && entryUrl.back() != '/') {
            entryUrl += "/";
        }
        entryUrl += name;

        // Format size with human-readable units
        std::string sizeStr;
        if (child->isDirectory()) {
            sizeStr = "-";
        } else {
            size_t size = child->size;
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
        if (child->epochNano > 0) {
            auto time = child->modifiedTime();
            timeStr = std::to_string(child->epochSeconds());
        }

        std::string displayName = name;
        if (child->isDirectory()) {
            displayName += "/";
        }

        // Get icon path
        std::string iconPath = m_fileTypeDetector.getColorIcon(name);
        if (child->isDirectory()) {
            iconPath = m_fileTypeDetector.getFolderColorIcon();
        } else {
            // For small image files (<4K), use the image itself as icon
            if (FileTypeDetector::isImageFile(name) && child->size > 0 &&
                child->size < PREVIEW_IMAGE_SIZE) {
                iconPath = entryUrl;
            }
        }

        // Get file description
        std::string description = "-";
        if (!child->isDirectory()) {
            description = FileTypeDetector::getFileDescription(name);
        }

        const std::string sortName = toLowerAscii(name + (child->isDirectory() ? "/" : ""));
        const long long sortMtime =
            child->epochNano > 0 ? static_cast<long long>(child->epochNano) : 0LL;
        const long long sortSize = child->isDirectory() ? 0LL : static_cast<long long>(child->size);
        const std::string sortDesc = toLowerAscii(description);

        html << "<tr data-sort-name=\"" << htmlAttrEscape(sortName) << "\" data-sort-mtime=\""
             << sortMtime << "\" data-sort-size=\"" << sortSize << "\" data-sort-desc=\""
             << htmlAttrEscape(sortDesc) << "\">"
             << "<td class=\"icon-cell\"><img src=\"" << iconPath
             << "\" class=\"icon\" alt=\"\"></td>"
             << "<td class=\"name\"><a href=\"" << entryUrl << "\">" << displayName << "</a></td>"
             << "<td class=\"date\">" << timeStr << "</td>"
             << "<td class=\"size\">" << sizeStr << "</td>"
             << "<td class=\"date\">" << description << "</td>"
             << "</tr>\n";
    }

    html << "</tbody>\n"
         << "</table>\n"
         << "<hr>\n"
         << "<script>\n"
         << "(function(){\n"
         << "  var tbody = document.getElementById('diridx-body');\n"
         << "  if (!tbody) return;\n"
         << "  var state = { key: 'name', dir: 1 };\n"
         << "  function cmp(a, b) {\n"
         << "    var k = state.key, d = state.dir;\n"
         << "    if (k === 'mtime') {\n"
         << "      var na = parseInt(a.dataset.sortMtime, 10) || 0, nb = "
            "parseInt(b.dataset.sortMtime, 10) || 0;\n"
         << "      return d * (na - nb);\n"
         << "    }\n"
         << "    if (k === 'size') {\n"
         << "      var na = parseInt(a.dataset.sortSize, 10) || 0, nb = "
            "parseInt(b.dataset.sortSize, 10) || 0;\n"
         << "      return d * (na - nb);\n"
         << "    }\n"
         << "    var va = k === 'name' ? (a.dataset.sortName || '') : (a.dataset.sortDesc || '');\n"
         << "    var vb = k === 'name' ? (b.dataset.sortName || '') : (b.dataset.sortDesc || '');\n"
         << "    return d * va.localeCompare(vb, undefined, { sensitivity: 'base' });\n"
         << "  }\n"
         << "  function resort() {\n"
         << "    var parentRow = tbody.querySelector('tr.diridx-parent');\n"
         << "    var rows = "
            "Array.prototype.slice.call(tbody.querySelectorAll('tr:not(.diridx-parent)'));\n"
         << "    rows.sort(cmp);\n"
         << "    if (parentRow && parentRow.parentNode === tbody) parentRow.remove();\n"
         << "    rows.forEach(function(r) { tbody.appendChild(r); });\n"
         << "    if (parentRow) tbody.insertBefore(parentRow, tbody.firstChild);\n"
         << "    document.querySelectorAll('th.diridx-th .sort-ind').forEach(function(s) { "
            "s.textContent = ''; });\n"
         << "    var th = document.querySelector('th.diridx-th[data-sort-key=\"' + state.key + "
            "'\"]');\n"
         << "    if (th) { var ind = th.querySelector('.sort-ind'); if (ind) ind.textContent = "
            "state.dir > 0 ? '\\u25b2' : '\\u25bc'; }\n"
         << "  }\n"
         << "  document.querySelectorAll('th.diridx-th').forEach(function(th) {\n"
         << "    function activate() {\n"
         << "      var k = th.getAttribute('data-sort-key');\n"
         << "      if (state.key === k) state.dir = -state.dir; else { state.key = k; state.dir = "
            "1; }\n"
         << "      resort();\n"
         << "    }\n"
         << "    th.addEventListener('click', activate);\n"
         << "    th.addEventListener('keydown', function(ev) {\n"
         << "      if (ev.key === 'Enter' || ev.key === ' ') { ev.preventDefault(); activate(); }\n"
         << "    });\n"
         << "  });\n"
         << "  resort();\n"
         << "})();\n"
         << "</script>\n"
         << "</body></html>\n";
    return html.str();
}

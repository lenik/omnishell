#include "VolumeIndex.hpp"

#include <bas/volume/VolumeManager.hpp>

#include <sstream>

VolumeIndex::VolumeIndex(VolumeManager* volumeManager)
    : m_volumeManager(volumeManager) {
}

std::string VolumeIndex::handleGet() {
    return generateHtml();
}

std::string VolumeIndex::generateHtml() {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html><head>\n"
         << "<title>Volumes</title>\n"
         << "<style type=\"text/css\">\n"
         << "body { font-family: \"DejaVu Sans\", \"Liberation Sans\", Arial, sans-serif; background-color: #fff; color: #000; margin: 20px; }\n"
         << "h1 { font-size: 1.5em; margin: 0 0 10px 0; }\n"
         << "hr { border: 0; border-top: 1px solid #ccc; margin: 20px 0; }\n"
         << "table { border-collapse: collapse; width: 100%; max-width: 100%; }\n"
         << "th { text-align: left; padding: 8px; background-color: #f0f0f0; border-bottom: 2px solid #ccc; font-weight: bold; }\n"
         << "td { padding: 8px; border-bottom: 1px solid #ddd; }\n"
         << "tr:hover { background-color: #f9f9f9; }\n"
         << "a { color: #0066cc; text-decoration: none; }\n"
         << "a:hover { text-decoration: underline; }\n"
         << ".name { text-align: left; }\n"
         << ".size { text-align: right; }\n"
         << ".date { text-align: left; }\n"
         << "</style>\n"
         << "</head><body>\n"
         << "<h1>Available Volumes</h1>\n"
         << "<hr>\n"
         << "<table>\n"
         << "<thead><tr>"
         << "<th class=\"name\">Index</th>"
         << "<th class=\"name\">Type</th>"
         << "<th class=\"name\">Label</th>"
         << "<th class=\"name\">UUID</th>"
         << "<th class=\"name\">Serial</th>"
         << "<th class=\"name\">Actions</th>"
         << "</tr></thead>\n"
         << "<tbody>\n";
    
    for (size_t i = 0; i < m_volumeManager->getVolumeCount(); ++i) {
        auto volume = m_volumeManager->getVolume(i);
        std::string uuid = volume->getUUID();
        std::string serial = volume->getSerial();
        
        // If UUID is empty, show "-"
        if (uuid.empty()) {
            uuid = "-";
        }
        
        // If serial is empty, show "-"
        if (serial.empty()) {
            serial = "-";
        }
        
        html << "<tr>"
             << "<td class=\"name\">" << i << "</td>"
             << "<td class=\"name\">" << volume->getTypeString() << "</td>"
             << "<td class=\"name\">" << volume->getLabel() << "</td>"
             << "<td class=\"name\">" << uuid << "</td>"
             << "<td class=\"name\">" << serial << "</td>"
             << "<td class=\"name\"><a href=\"/volume/" << i << "/\">Browse</a></td>"
             << "</tr>\n";
    }
    
    html << "</tbody>\n"
         << "</table>\n"
         << "<hr>\n"
         << "</body></html>\n";
    return html.str();
}

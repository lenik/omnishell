#include "BreadcrumbNav.hpp"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

BreadcrumbNav::BreadcrumbNav(wxWindow* parent, const std::string& location)
    : os::wxcPanel(parent, wxID_ANY), m_location(location) {

    m_sizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(m_sizer);

    Bind(wxEVT_BUTTON, &BreadcrumbNav::OnBreadcrumbClicked, this);

    updateBreadcrumbs();
}

BreadcrumbNav::~BreadcrumbNav() {
}

void BreadcrumbNav::setLocation(const std::string& location) {
    if (m_location != location) {
        m_location = location;
        updateBreadcrumbs();
    }
}

void BreadcrumbNav::updateBreadcrumbs() {
    clearBreadcrumbs();
    
    if (m_location.empty()) {
        return;
    }
    
    std::vector<std::string> pathComponents = splitPath(m_location);
    
    // Add root button
    wxButton* rootButton = new wxButton(this, wxID_ANY, "Root", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    rootButton->SetClientData(new std::string("/"));
    m_breadcrumbButtons.push_back(rootButton);
    m_sizer->Add(rootButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    
    // Add separator if there are more components
    if (!pathComponents.empty()) {
        wxStaticText* separator = new wxStaticText(this, wxID_ANY, " > ");
        m_sizer->Add(separator, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    }
    
    // Add path component buttons
    std::string currentPath = "";
    for (size_t i = 0; i < pathComponents.size(); ++i) {
        currentPath += "/" + pathComponents[i];
        
        wxButton* button = new wxButton(this, wxID_ANY, pathComponents[i], 
                                       wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
        button->SetClientData(new std::string(currentPath));
        m_breadcrumbButtons.push_back(button);
        m_sizer->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
        
        // Add separator if not the last component
        if (i < pathComponents.size() - 1) {
            wxStaticText* separator = new wxStaticText(this, wxID_ANY, " > ");
            m_sizer->Add(separator, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
        }
    }
    
    Layout();
}

void BreadcrumbNav::clearBreadcrumbs() {
    // Free client-owned path strings; window destruction is done once via sizer->Clear(true).
    // Do not call Destroy() on each button here — Clear(true) already deletes all sizer children,
    // and double-destroying causes heap corruption / std::terminate on the next GTK event.
    for (wxButton* button : m_breadcrumbButtons) {
        auto* path = static_cast<std::string*>(button->GetClientData());
        delete path;
        button->SetClientData(nullptr);
    }
    m_breadcrumbButtons.clear();
    m_sizer->Clear(true);
}

std::vector<std::string> BreadcrumbNav::splitPath(const std::string& path) {
    std::vector<std::string> components;
    
    if (path.empty() || path == "/") {
        return components;
    }
    
    std::string workingPath = path;
    
    // Remove leading slash
    if (workingPath.front() == '/') {
        workingPath = workingPath.substr(1);
    }
    
    // Remove trailing slash
    if (!workingPath.empty() && workingPath.back() == '/') {
        workingPath.pop_back();
    }
    
    // Split by '/'
    size_t pos = 0;
    while ((pos = workingPath.find('/')) != std::string::npos) {
        std::string component = workingPath.substr(0, pos);
        if (!component.empty()) {
            components.push_back(component);
        }
        workingPath = workingPath.substr(pos + 1);
    }
    
    // Add the last component
    if (!workingPath.empty()) {
        components.push_back(workingPath);
    }
    
    return components;
}

void BreadcrumbNav::OnBreadcrumbClicked(wxCommandEvent& event) {
    wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
    if (!button)
        return;

    std::string* path = static_cast<std::string*>(button->GetClientData());
    if (!path)
        return;
    notifyPathSelected(*path);
    event.Skip();
}

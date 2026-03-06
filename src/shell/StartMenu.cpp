#include "StartMenu.hpp"

#include <wx/log.h>
#include <wx/statline.h>

#include <algorithm>

namespace os {

StartMenu::StartMenu(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Start Menu",
                wxDefaultPosition, wxSize(400, 500),
                wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxBORDER_SIMPLE)
    , searchHeight_(40)
    , itemHeight_(40)
    , menuWidth_(300)
    , menuHeight_(400)
    , launchCallback_(nullptr)
{
    SetTransparent(245);
    SetBackgroundColour(wxColour(240, 240, 240));
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Search box
    searchBox_ = new wxTextCtrl(this, wxID_ANY, "Search programs...",
                                 wxDefaultPosition, wxSize(-1, searchHeight_),
                                 wxTE_PROCESS_ENTER);
    searchBox_->Bind(wxEVT_TEXT, &StartMenu::OnSearch, this);
    searchBox_->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) {
        // Launch first filtered item
        if (!filteredModules_.empty()) {
            if (launchCallback_) {
                launchCallback_(filteredModules_[0]);
            }
            Show(false);
        }
    });
    searchBox_->Bind(wxEVT_KEY_DOWN, &StartMenu::OnKeyDown, this);
    
    mainSizer->Add(searchBox_, 0, wxEXPAND | wxALL, 5);
    
    // Module list
    moduleList_ = new wxListBox(this, wxID_ANY,
                                 wxDefaultPosition, wxSize(-1, 300),
                                 0, nullptr,
                                 wxLB_SINGLE | wxLB_NO_SB);
    moduleList_->Bind(wxEVT_LISTBOX, &StartMenu::OnModuleSelected, this);
    moduleList_->Bind(wxEVT_LISTBOX_DCLICK, &StartMenu::OnModuleDoubleClicked, this);
    
    mainSizer->Add(moduleList_, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    
    // Separator
    mainSizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxALL, 5);
    
    // Category panel (placeholder for future categories)
    categoryPanel_ = new wxPanel(this, wxID_ANY);
    wxBoxSizer* categorySizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxStaticText* catLabel = new wxStaticText(categoryPanel_, wxID_ANY, "Categories:");
    categorySizer->Add(catLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    // Add some category buttons (placeholder)
    const char* categories[] = {"All", "Accessories", "System", "Settings", nullptr};
    for (int i = 0; categories[i] != nullptr; i++) {
        wxButton* btn = new wxButton(categoryPanel_, wxID_ANY, categories[i]);
        btn->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent&) {
            // TODO: Filter by category
            wxLogInfo("Category %d clicked", i);
        });
        categorySizer->Add(btn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    }
    
    categoryPanel_->SetSizer(categorySizer);
    mainSizer->Add(categoryPanel_, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
    Layout();
    
    // Bind close event
    Bind(wxEVT_CLOSE_WINDOW, &StartMenu::OnClose, this);
}

StartMenu::~StartMenu() {
}

void StartMenu::ShowAt(const wxPoint& position) {
    SetPosition(position);
    Show(true);
    searchBox_->SetFocus();
    searchBox_->SelectAll();
}

void StartMenu::populateModules(const std::vector<ModulePtr>& modules) {
    allModules_ = modules;
    filteredModules_ = modules;
    CreateModuleList();
}

void StartMenu::clearModules() {
    allModules_.clear();
    filteredModules_.clear();
    moduleList_->Clear();
}

void StartMenu::setLaunchCallback(LaunchCallback callback) {
    launchCallback_ = callback;
}

void StartMenu::OnSearch(wxCommandEvent& event) {
    wxString searchText = searchBox_->GetValue();
    
    if (searchText.IsEmpty() || searchText == "Search programs...") {
        filteredModules_ = allModules_;
    } else {
        FilterModules(searchText.ToStdString());
    }
    
    CreateModuleList();
    event.Skip();
}

void StartMenu::OnModuleSelected(wxCommandEvent& event) {
    // Highlight selected item
    event.Skip();
}

void StartMenu::OnModuleDoubleClicked(wxCommandEvent& event) {
    int selection = moduleList_->GetSelection();
    if (selection != wxNOT_FOUND && selection < (int)filteredModules_.size()) {
        ModulePtr module = filteredModules_[selection];
        if (launchCallback_) {
            launchCallback_(module);
        }
        Show(false);
    }
    event.Skip();
}

void StartMenu::OnKeyDown(wxKeyEvent& event) {
    switch (event.GetKeyCode()) {
        case WXK_DOWN:
            if (moduleList_->GetCount() > 0) {
                moduleList_->SetSelection(0);
                moduleList_->SetFocus();
            }
            break;
        case WXK_ESCAPE:
            Show(false);
            break;
        default:
            event.Skip();
            break;
    }
}

void StartMenu::OnClose(wxCloseEvent& event) {
    Show(false);
}

void StartMenu::FilterModules(const std::string& searchText) {
    filteredModules_.clear();
    
    std::string lowerSearch = searchText;
    std::transform(lowerSearch.begin(), lowerSearch.end(), 
                   lowerSearch.begin(), ::tolower);
    
    for (const auto& module : allModules_) {
        if (!module->isVisible()) continue;
        
        std::string name = module->name;
        std::string label = module->label;
        std::string desc = module->description;
        
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::transform(label.begin(), label.end(), label.begin(), ::tolower);
        std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);
        
        if (name.find(lowerSearch) != std::string::npos ||
            label.find(lowerSearch) != std::string::npos ||
            desc.find(lowerSearch) != std::string::npos) {
            filteredModules_.push_back(module);
        }
    }
}

void StartMenu::CreateModuleList() {
    moduleList_->Clear();
    
    for (const auto& module : filteredModules_) {
        wxString item = module->label;
        if (!module->description.empty()) {
            item += " - " + module->description;
        }
        moduleList_->Append(item);
    }
    
    if (moduleList_->GetCount() > 0) {
        moduleList_->SetSelection(0);
    }
}

} // namespace os

#include "ControlPanelApp.hpp"

#include "../../core/ModuleRegistry.hpp"

#include <wx/log.h>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.controlpanel", ControlPanelApp)

ControlPanelApp::ControlPanelApp()
    : frame_(nullptr), categoryList_(nullptr), contentPanel_(nullptr) {
    initializeMetadata();
}

ControlPanelApp::~ControlPanelApp() {
    if (frame_) {
        frame_->Destroy();
    }
}

void ControlPanelApp::initializeMetadata() {
    uri = "omnishell";
    name = "controlpanel";
    label = "Control Panel";
    description = "System configuration and settings";
    doc = "Configure system settings, manage modules, and customize your OmniShell environment.";

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "cog-1.svg"))
                .scale(16, 16, Path(dir, "cog-1-16x16.png"))
                .scale(24, 24, Path(dir, "cog-1-24x24.png"))
                .scale(32, 32, Path(dir, "cog-1-32x32.png"))
                .scale(48, 48, Path(dir, "cog-1-48x48.png"));
}

void ControlPanelApp::run() {
    if (frame_) {
        frame_->Raise();
        frame_->SetFocus();
        return;
    }

    createMainWindow();
    frame_->Show(true);
}

void ControlPanelApp::createMainWindow() {
    frame_ = new wxFrame(nullptr, wxID_ANY, "Control Panel", wxDefaultPosition, wxSize(900, 600));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Left panel - Category list
    wxPanel* leftPanel = new wxPanel(frame_, wxID_ANY);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* label = new wxStaticText(leftPanel, wxID_ANY, "Categories:");
    leftSizer->Add(label, 0, wxALL, 5);

    categoryList_ = new wxListView(leftPanel, wxID_ANY, wxDefaultPosition, wxSize(200, 400),
                                   wxLC_LIST | wxLC_SINGLE_SEL);

    // Add categories
    categories_ = {{"Module Manager", "Manage installed modules", 0},
                   {"System Settings", "Configure system options", 1},
                   {"User Configuration", "User preferences", 2},
                   {"Desktop", "Desktop appearance", 3},
                   {"Display", "Screen settings", 4},
                   {"About", "System information", 5}};

    for (size_t i = 0; i < categories_.size(); i++) {
        categoryList_->InsertItem(i, categories_[i].name);
    }

    categoryList_->Bind(wxEVT_LIST_ITEM_SELECTED, &ControlPanelApp::OnCategorySelected, this);
    categoryList_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ControlPanelApp::OnModuleDoubleClicked, this);

    leftSizer->Add(categoryList_, 1, wxEXPAND | wxALL, 5);
    leftPanel->SetSizer(leftSizer);

    // Right panel - Content
    contentPanel_ = new wxPanel(frame_, wxID_ANY);
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* contentLabel =
        new wxStaticText(contentPanel_, wxID_ANY, "Select a category to view options");
    contentLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    contentSizer->Add(contentLabel, 0, wxALL | wxALIGN_CENTER, 20);

    contentPanel_->SetSizer(contentSizer);

    mainSizer->Add(leftPanel, 0, wxEXPAND);
    mainSizer->Add(contentPanel_, 1, wxEXPAND);

    frame_->SetSizer(mainSizer);
    frame_->Centre();
}

void ControlPanelApp::createCategoryList() {
    // Already done in createMainWindow
}

void ControlPanelApp::createContentPanel() {
    // Already done in createMainWindow
}

void ControlPanelApp::OnCategorySelected(wxCommandEvent& event) {
    int selection = categoryList_->GetSelectedItemCount();
    if (selection == wxNOT_FOUND)
        return;

    // Get selected item
    long item = -1;
    item = categoryList_->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    if (item != -1 && item < (long)categories_.size()) {
        wxString category = categories_[item].name;

        // Clear content panel
        contentPanel_->GetSizer()->Clear(true);

        // Show appropriate content
        if (category == "Module Manager") {
            showModuleManager();
        } else if (category == "System Settings") {
            showSystemSettings();
        } else if (category == "User Configuration") {
            showUserConfiguration();
        } else {
            wxStaticText* label =
                new wxStaticText(contentPanel_, wxID_ANY, category + " (not implemented)");
            contentPanel_->GetSizer()->Add(label, 0, wxALL | wxALIGN_CENTER, 20);
        }

        contentPanel_->Layout();
    }

    event.Skip();
}

void ControlPanelApp::OnModuleDoubleClicked(wxCommandEvent& event) { OnCategorySelected(event); }

void ControlPanelApp::showModuleManager() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* label = new wxStaticText(contentPanel_, wxID_ANY, "Installed Modules:");
    sizer->Add(label, 0, wxALL, 5);

    // List all modules
    wxListView* moduleList = new wxListView(contentPanel_, wxID_ANY, wxDefaultPosition,
                                            wxSize(400, 300), wxLC_REPORT | wxLC_SINGLE_SEL);

    moduleList->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 150);
    moduleList->InsertColumn(1, "Description", wxLIST_FORMAT_LEFT, 200);
    moduleList->InsertColumn(2, "Status", wxLIST_FORMAT_LEFT, 100);

    auto& registry = ModuleRegistry::getInstance();
    auto modules = registry.getAllModules();

    int index = 0;
    for (const auto& module : modules) {
        moduleList->InsertItem(index, module->label);
        moduleList->SetItem(index, 1, module->description);
        moduleList->SetItem(index, 2, module->isEnabled() ? "Enabled" : "Disabled");
        index++;
    }

    sizer->Add(moduleList, 1, wxEXPAND | wxALL, 5);

    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* enableBtn = new wxButton(contentPanel_, wxID_ANY, "Enable");
    wxButton* disableBtn = new wxButton(contentPanel_, wxID_ANY, "Disable");
    wxButton* closeBtn = new wxButton(contentPanel_, wxID_CLOSE, "Close");

    enableBtn->Bind(wxEVT_BUTTON, [this, moduleList](wxCommandEvent&) {
        long sel = moduleList->GetSelectedItemCount();
        if (sel > 0) {
            wxMessageBox("Enable module (not implemented)", "Control Panel",
                         wxOK | wxICON_INFORMATION);
        }
    });

    disableBtn->Bind(wxEVT_BUTTON, [this, moduleList](wxCommandEvent&) {
        long sel = moduleList->GetSelectedItemCount();
        if (sel > 0) {
            wxMessageBox("Disable module (not implemented)", "Control Panel",
                         wxOK | wxICON_INFORMATION);
        }
    });

    closeBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { frame_->Close(); });

    buttonSizer->Add(enableBtn, 0, wxALL, 5);
    buttonSizer->Add(disableBtn, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(closeBtn, 0, wxALL, 5);

    sizer->Add(buttonSizer, 0, wxEXPAND);

    contentPanel_->GetSizer()->Clear(true);
    contentPanel_->SetSizer(sizer);
}

void ControlPanelApp::showSystemSettings() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* label =
        new wxStaticText(contentPanel_, wxID_ANY, "System Settings (not implemented)");
    sizer->Add(label, 0, wxALL | wxALIGN_CENTER, 20);

    contentPanel_->GetSizer()->Clear(true);
    contentPanel_->SetSizer(sizer);
}

void ControlPanelApp::showUserConfiguration() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* label =
        new wxStaticText(contentPanel_, wxID_ANY, "User Configuration (not implemented)");
    sizer->Add(label, 0, wxALL | wxALIGN_CENTER, 20);

    contentPanel_->GetSizer()->Clear(true);
    contentPanel_->SetSizer(sizer);
}

} // namespace os

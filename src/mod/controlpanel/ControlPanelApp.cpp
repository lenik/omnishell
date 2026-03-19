#include "ControlPanelApp.hpp"

#include "../../core/ModuleRegistry.hpp"
#include "../../core/RegistryDb.hpp"
#include "../../shell/Shell.hpp"

#include <wx/log.h>

namespace os {

OMNISHELL_REGISTER_MODULE("omnishell.controlpanel", ControlPanelApp)

ControlPanelApp::ControlPanelApp(CreateModuleContext* ctx)
    : Module(ctx)
    , m_frame(nullptr)
    , m_categoryList(nullptr)
    , m_contentPanel(nullptr) {
    initializeMetadata();
}

ControlPanelApp::~ControlPanelApp() {
    if (m_frame) {
        m_frame->Destroy();
    }
}

void ControlPanelApp::initializeMetadata() {
    uri = "omnishell";
    name = "controlpanel";
    label = "Control Panel";
    description = "System configuration and settings";
    doc = "Configure system settings, manage modules, and customize your OmniShell environment.";
    categoryId = ID_CATEGORY_SYSTEM;

    std::string dir = "streamline-vectors/core/pop/interface-essential";
    image = ImageSet(Path(dir, "cog-1.svg"));
}

ProcessPtr ControlPanelApp::run() {
    if (m_frame) {
        m_frame->Raise();
        m_frame->SetFocus();
        auto p = std::make_shared<Process>();
        p->uri = uri;
        p->name = name;
        p->label = label;
        p->icon = image;
        p->addWindow(m_frame);
        return p;
    }

    createMainWindow();
    m_frame->Show(true);
    auto p = std::make_shared<Process>();
    p->uri = uri;
    p->name = name;
    p->label = label;
    p->icon = image;
    p->addWindow(m_frame);
    return p;
}

void ControlPanelApp::createMainWindow() {
    m_frame = new wxFrame(nullptr, wxID_ANY, "Control Panel", wxDefaultPosition, wxSize(900, 600));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Left panel - Category list
    wxPanel* leftPanel = new wxPanel(m_frame, wxID_ANY);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* label = new wxStaticText(leftPanel, wxID_ANY, "Categories:");
    leftSizer->Add(label, 0, wxALL, 5);

    m_categoryList = new wxListView(leftPanel, wxID_ANY, wxDefaultPosition, wxSize(200, 400),
                                   wxLC_LIST | wxLC_SINGLE_SEL);

    // Add categories
    m_categories = {{"Module Manager", "Manage installed modules", 0},
                   {"System Settings", "Configure system options", 1},
                   {"User Configuration", "User preferences", 2},
                   {"Desktop", "Desktop appearance", 3},
                   {"Display", "Screen settings", 4},
                   {"About", "System information", 5}};

    for (size_t i = 0; i < m_categories.size(); i++) {
        m_categoryList->InsertItem(i, m_categories[i].name);
    }

    m_categoryList->Bind(wxEVT_LIST_ITEM_SELECTED, &ControlPanelApp::OnCategorySelected, this);
    m_categoryList->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ControlPanelApp::OnModuleDoubleClicked, this);

    leftSizer->Add(m_categoryList, 1, wxEXPAND | wxALL, 5);
    leftPanel->SetSizer(leftSizer);

    // Right panel - Content
    m_contentPanel = new wxPanel(m_frame, wxID_ANY);
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* contentLabel =
        new wxStaticText(m_contentPanel, wxID_ANY, "Select a category to view options");
    contentLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    contentLabel->Wrap(560);
    contentSizer->Add(contentLabel, 0, wxALL | wxEXPAND, 20);

    m_contentPanel->SetSizer(contentSizer);

    mainSizer->Add(leftPanel, 0, wxEXPAND);
    mainSizer->Add(m_contentPanel, 1, wxEXPAND);

    m_frame->SetSizer(mainSizer);
    m_frame->Centre();
}

void ControlPanelApp::createCategoryList() {
    // Already done in createMainWindow
}

void ControlPanelApp::createContentPanel() {
    // Already done in createMainWindow
}

void ControlPanelApp::OnCategorySelected(wxCommandEvent& event) {
    int selection = m_categoryList->GetSelectedItemCount();
    if (selection == wxNOT_FOUND)
        return;

    // Get selected item
    long item = -1;
    item = m_categoryList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    if (item != -1 && item < (long)m_categories.size()) {
        wxString category = m_categories[item].name;

        // Clear content panel
        m_contentPanel->GetSizer()->Clear(true);

        // Show appropriate content
        if (category == "Module Manager") {
            showModuleManager();
        } else if (category == "System Settings") {
            showSystemSettings();
        } else if (category == "User Configuration") {
            showUserConfiguration();
        } else if (category == "Desktop") {
            showDesktopSettings();
        } else if (category == "Display") {
            showDisplaySettings();
        } else if (category == "About") {
            showAbout();
        }

        m_contentPanel->Layout();
    }

    event.Skip();
}

void ControlPanelApp::OnModuleDoubleClicked(wxCommandEvent& event) { OnCategorySelected(event); }

void ControlPanelApp::showModuleManager() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* label = new wxStaticText(m_contentPanel, wxID_ANY, "Installed Modules:");
    label->Wrap(560);
    sizer->Add(label, 0, wxALL, 5);

    // List all modules
    wxListView* moduleList = new wxListView(m_contentPanel, wxID_ANY, wxDefaultPosition,
                                            wxSize(400, 300), wxLC_REPORT | wxLC_SINGLE_SEL);

    moduleList->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 150);
    moduleList->InsertColumn(1, "Description", wxLIST_FORMAT_LEFT, 200);
    moduleList->InsertColumn(2, "Status", wxLIST_FORMAT_LEFT, 100);

    auto* shell = ShellApp::getInstance();
    auto* registry = shell ? shell->getModuleRegistry() : nullptr;
    std::vector<ModulePtr> modules;
    if (registry) {
        modules = registry->getAllModules();
    }

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

    wxButton* enableBtn = new wxButton(m_contentPanel, wxID_ANY, "Enable");
    wxButton* disableBtn = new wxButton(m_contentPanel, wxID_ANY, "Disable");
    wxButton* closeBtn = new wxButton(m_contentPanel, wxID_CLOSE, "Close");

    auto refreshStatus = [moduleList, modules]() {
        for (int i = 0; i < (int)modules.size(); ++i) {
            const auto& m = modules[i];
            moduleList->SetItem(i, 2, (m && m->isEnabled()) ? "Enabled" : "Disabled");
        }
    };

    enableBtn->Bind(wxEVT_BUTTON, [this, moduleList, modules, refreshStatus](wxCommandEvent&) {
        long item = -1;
        item = moduleList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == -1 || item >= (long)modules.size())
            return;
        auto m = modules[(size_t)item];
        if (!m)
            return;
        RegistryDb::getInstance().set("Module.Disabled." + m->getFullUri(), "0");
        RegistryDb::getInstance().save();
        refreshStatus();
    });

    disableBtn->Bind(wxEVT_BUTTON, [this, moduleList, modules, refreshStatus](wxCommandEvent&) {
        long item = -1;
        item = moduleList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == -1 || item >= (long)modules.size())
            return;
        auto m = modules[(size_t)item];
        if (!m)
            return;
        RegistryDb::getInstance().set("Module.Disabled." + m->getFullUri(), "1");
        RegistryDb::getInstance().save();
        refreshStatus();
    });

    closeBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { m_frame->Close(); });

    buttonSizer->Add(enableBtn, 0, wxALL, 5);
    buttonSizer->Add(disableBtn, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(closeBtn, 0, wxALL, 5);

    sizer->Add(buttonSizer, 0, wxEXPAND);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

void ControlPanelApp::showSystemSettings() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    auto& reg = RegistryDb::getInstance();
    reg.load();
    wxString osName = reg.get("System.OS.Name", "Omnishell");
    wxString osVer = reg.get("System.OS.Version", "1.1.1");

    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, "System");
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->Wrap(560);
    sizer->Add(title, 0, wxALL | wxEXPAND, 10);

    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "OS Name: " + osName), 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "OS Version: " + osVer), 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

void ControlPanelApp::showUserConfiguration() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    auto& reg = RegistryDb::getInstance();
    reg.load();
    wxString userName = reg.get("User.Name", "Guest");
    wxString userRole = reg.get("User.Role", "Guest");

    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, "User");
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->Wrap(560);
    sizer->Add(title, 0, wxALL | wxEXPAND, 10);

    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Name: " + userName), 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Role: " + userRole), 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

void ControlPanelApp::showDesktopSettings() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, "Desktop");
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->Wrap(560);
    sizer->Add(title, 0, wxALL | wxEXPAND, 10);

    wxButton* bgBtn = new wxButton(m_contentPanel, wxID_ANY, "Background Settings");
    bgBtn->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        auto shell = ShellApp::getInstance();
        if (!shell || !shell->getModuleRegistry())
            return;
        auto mods = shell->getModuleRegistry()->getVisibleModules();
        for (auto& m : mods) {
            if (m && m->name == "backgroundsettings") {
                (void)m->run();
                return;
            }
        }
    });
    sizer->Add(bgBtn, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    wxButton* arrangeBtn = new wxButton(m_contentPanel, wxID_ANY, "Arrange Desktop Icons");
    arrangeBtn->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        auto shell = ShellApp::getInstance();
        if (shell && shell->getDesktopWindow())
            shell->getDesktopWindow()->arrangeIcons();
    });
    sizer->Add(arrangeBtn, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

void ControlPanelApp::showDisplaySettings() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, "Display");
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->Wrap(560);
    sizer->Add(title, 0, wxALL | wxEXPAND, 10);

    wxSize sz = wxGetDisplaySize();
    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY,
                                wxString::Format("Resolution: %d x %d", sz.GetWidth(), sz.GetHeight())),
              0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

void ControlPanelApp::showAbout() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, "About OmniShell");
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->Wrap(560);
    sizer->Add(title, 0, wxALL | wxEXPAND, 10);

    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "OmniShell Desktop Environment"), 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Version: 1.1.1"), 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

} // namespace os

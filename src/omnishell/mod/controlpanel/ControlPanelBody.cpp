#include "ControlPanelBody.hpp"

#include "../../core/App.hpp"
#include "../../core/ModuleRegistry.hpp"
#include "../../core/RegistryDb.hpp"
#include "../../shell/Shell.hpp"
#include "../../wx/wxcWindow.hpp"

#include <wx/intl.h>

namespace os {

ControlPanelBody::ControlPanelBody(App* app) {
    (void)app;
}

void ControlPanelBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    uiFrame* frame = dynamic_cast<uiFrame*>(parent);
    if (!frame)
        return;
    m_frame = frame;

    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    wxPanel* leftPanel = new wxPanel(parent, wxID_ANY);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* label = new wxStaticText(leftPanel, wxID_ANY, _("Categories:"));
    leftSizer->Add(label, 0, wxALL, 5);

    m_categoryList = new wxListView(leftPanel, wxID_ANY, wxDefaultPosition, wxSize(200, 400),
                                    wxLC_LIST | wxLC_SINGLE_SEL);

    m_categories = {{PanelCategory::ModuleManager, _("Module Manager"), _("Manage installed modules"), 0},
                    {PanelCategory::SystemSettings, _("System Settings"), _("Configure system options"), 1},
                    {PanelCategory::UserConfiguration, _("User Configuration"), _("User preferences"), 2},
                    {PanelCategory::Desktop, _("Desktop"), _("Desktop appearance"), 3},
                    {PanelCategory::Display, _("Display"), _("Screen settings"), 4},
                    {PanelCategory::About, _("About"), _("System information"), 5}};

    for (size_t i = 0; i < m_categories.size(); i++) {
        m_categoryList->InsertItem(i, m_categories[i].name);
    }

    wxcBind(*m_categoryList, wxEVT_LIST_ITEM_SELECTED,
            &ControlPanelBody::OnCategorySelected, this);
    wxcBind(*m_categoryList, wxEVT_LIST_ITEM_ACTIVATED,
            &ControlPanelBody::OnModuleDoubleClicked, this);

    leftSizer->Add(m_categoryList, 1, wxEXPAND | wxALL, 5);
    leftPanel->SetSizer(leftSizer);

    m_contentPanel = new wxPanel(parent, wxID_ANY);
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* contentLabel =
        new wxStaticText(m_contentPanel, wxID_ANY, _("Select a category to view options"));
    contentLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    contentLabel->Wrap(560);
    contentSizer->Add(contentLabel, 0, wxALL | wxEXPAND, 20);

    m_contentPanel->SetSizer(contentSizer);

    mainSizer->Add(leftPanel, 0, wxEXPAND);
    mainSizer->Add(m_contentPanel, 1, wxEXPAND);

    parent->SetSizer(mainSizer);
}

wxEvtHandler* ControlPanelBody::getEventHandler() {
    return m_frame ? m_frame->GetEventHandler() : nullptr;
}

void ControlPanelBody::OnCategorySelected(wxCommandEvent& event) {
    int selection = m_categoryList->GetSelectedItemCount();
    if (selection == wxNOT_FOUND)
        return;

    long item = -1;
    item = m_categoryList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    if (item != -1 && item < (long)m_categories.size()) {
        m_contentPanel->GetSizer()->Clear(true);

        switch (m_categories[(size_t)item].id) {
        case PanelCategory::ModuleManager:
            showModuleManager();
            break;
        case PanelCategory::SystemSettings:
            showSystemSettings();
            break;
        case PanelCategory::UserConfiguration:
            showUserConfiguration();
            break;
        case PanelCategory::Desktop:
            showDesktopSettings();
            break;
        case PanelCategory::Display:
            showDisplaySettings();
            break;
        case PanelCategory::About:
            showAbout();
            break;
        }

        m_contentPanel->Layout();
    }
    event.Skip();
}

void ControlPanelBody::OnModuleDoubleClicked(wxCommandEvent& event) { OnCategorySelected(event); }

void ControlPanelBody::showModuleManager() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* label = new wxStaticText(m_contentPanel, wxID_ANY, _("Installed Modules:"));
    label->Wrap(560);
    sizer->Add(label, 0, wxALL, 5);

    wxListView* moduleList = new wxListView(m_contentPanel, wxID_ANY, wxDefaultPosition,
                                            wxSize(400, 300), wxLC_REPORT | wxLC_SINGLE_SEL);
    moduleList->SetName(wxT("module_list"));

    moduleList->InsertColumn(0, _("Name"), wxLIST_FORMAT_LEFT, 150);
    moduleList->InsertColumn(1, _("Description"), wxLIST_FORMAT_LEFT, 200);
    moduleList->InsertColumn(2, _("Status"), wxLIST_FORMAT_LEFT, 100);

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
        moduleList->SetItem(index, 2, module->isEnabled() ? _("Enabled") : _("Disabled"));
        index++;
    }

    sizer->Add(moduleList, 1, wxEXPAND | wxALL, 5);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* enableBtn = new wxButton(m_contentPanel, wxID_ANY, _("Enable"));
    enableBtn->SetName(wxT("enable_module"));
    wxButton* disableBtn = new wxButton(m_contentPanel, wxID_ANY, _("Disable"));
    disableBtn->SetName(wxT("disable_module"));
    wxButton* closeBtn = new wxButton(m_contentPanel, wxID_CLOSE, _("Close"));
    closeBtn->SetName(wxT("close"));

    auto refreshStatus = [moduleList, modules]() {
        for (int i = 0; i < (int)modules.size(); ++i) {
            const auto& m = modules[i];
            moduleList->SetItem(i, 2, (m && m->isEnabled()) ? _("Enabled") : _("Disabled"));
        }
    };

    wxcBind(*enableBtn, wxEVT_BUTTON,
            [moduleList, modules, refreshStatus](wxCommandEvent&) {
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

    wxcBind(*disableBtn, wxEVT_BUTTON,
            [moduleList, modules, refreshStatus](wxCommandEvent&) {
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

    wxcBind(*closeBtn, wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_frame)
            m_frame->Close();
    });

    buttonSizer->Add(enableBtn, 0, wxALL, 5);
    buttonSizer->Add(disableBtn, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(closeBtn, 0, wxALL, 5);

    sizer->Add(buttonSizer, 0, wxEXPAND);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

void ControlPanelBody::showSystemSettings() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    auto& reg = RegistryDb::getInstance();
    reg.load();
    wxString osName = reg.getString("System.OS.Name", "Omnishell");
    wxString osVer = reg.getString("System.OS.Version", "1.1.1");

    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, _("System"));
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->Wrap(560);
    sizer->Add(title, 0, wxALL | wxEXPAND, 10);

    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, wxString::Format(_("OS Name: %s"), osName)),
               0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, wxString::Format(_("OS Version: %s"), osVer)),
               0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

void ControlPanelBody::showUserConfiguration() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    auto& reg = RegistryDb::getInstance();
    reg.load();
    wxString userName = reg.getString("User.Name", "Guest");
    wxString userRole = reg.getString("User.Role", "Guest");

    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, _("User"));
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->Wrap(560);
    sizer->Add(title, 0, wxALL | wxEXPAND, 10);

    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, wxString::Format(_("Name: %s"), userName)),
               0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, wxString::Format(_("Role: %s"), userRole)),
               0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

void ControlPanelBody::showDesktopSettings() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, _("Desktop"));
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->Wrap(560);
    sizer->Add(title, 0, wxALL | wxEXPAND, 10);

    wxButton* bgBtn = new wxButton(m_contentPanel, wxID_ANY, _("Background Settings"));
    bgBtn->SetName(wxT("background_settings"));
    wxcBind(*bgBtn, wxEVT_BUTTON, [](wxCommandEvent&) {
        auto shell = ShellApp::getInstance();
        if (!shell || !shell->getModuleRegistry())
            return;
        auto mods = shell->getModuleRegistry()->getVisibleModules();
        for (auto& m : mods) {
            if (m && m->name == "backgroundsettings") {
                (void)m->run(app.makeRunConfig());
                return;
            }
        }
    });
    sizer->Add(bgBtn, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    wxButton* arrangeBtn = new wxButton(m_contentPanel, wxID_ANY, _("Arrange Desktop Icons"));
    arrangeBtn->SetName(wxT("arrange_desktop"));
    wxcBind(*arrangeBtn, wxEVT_BUTTON, [](wxCommandEvent&) {
        auto shell = ShellApp::getInstance();
        if (shell && shell->getDesktopWindow())
            shell->getDesktopWindow()->arrangeIcons();
    });
    sizer->Add(arrangeBtn, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

void ControlPanelBody::showDisplaySettings() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, _("Display"));
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->Wrap(560);
    sizer->Add(title, 0, wxALL | wxEXPAND, 10);

    wxSize sz = wxGetDisplaySize();
    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY,
                                wxString::Format(_("Resolution: %d x %d"), sz.GetWidth(), sz.GetHeight())),
               0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

void ControlPanelBody::showAbout() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, _("About OmniShell"));
    title->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    title->Wrap(560);
    sizer->Add(title, 0, wxALL | wxEXPAND, 10);

    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, _("OmniShell Desktop Environment")),
               0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    sizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, _("Version: 1.1.1")), 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    m_contentPanel->GetSizer()->Clear(true);
    m_contentPanel->SetSizer(sizer);
}

} // namespace os

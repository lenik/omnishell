#include "Shell.hpp"

#include "../core/App.hpp"
#include "../core/LocaleSetup.hpp"
#include "../core/ModuleRegistry.hpp"
#include "../core/ServiceManager.hpp"
#include "../core/VolUrl.hpp"
#include "../core/registry/FileAssociations.hpp"
#include "../mod/explorer/ExplorerApp.hpp"

#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeFile.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/image.h>
#include <wx/log.h>
#include <wx/splitter.h>
#include <wx/statline.h>

namespace os {

ShellApp* ShellApp::m_instance = nullptr;

ShellApp::ShellApp(std::string name)
    : m_name(std::move(name))
    , m_volumeManager(nullptr)
    , m_moduleRegistry(nullptr)
    , m_mainWindow(nullptr)
    , m_desktop(nullptr)
    , m_taskbar(nullptr)
    , m_startMenu(nullptr) {
    m_instance = this;
}

ShellApp::~ShellApp() {
    ServiceManager::getInstance().stopAllServices();
    if (m_moduleRegistry) {
        m_moduleRegistry->uninstallAll();
        delete m_moduleRegistry;
        m_moduleRegistry = nullptr;
    }
    m_instance = nullptr;
}

bool ShellApp::OnUserInit() {
    (void)setupOmniShellLocale(m_locale);

    m_volumeManager = app.volumeManager.get();
    if (!m_volumeManager) {
        wxLogError("VolumeManager not initialized");
        return false;
    }

    if (!m_vfsDaemon.start(m_volumeManager)) {
        wxLogWarning("VFS HTTP daemon failed to start; Browser and MediaPlayer VFS URLs may not work.");
    }

    SetAppName(m_name);
    SetAppDisplayName(m_name + " Desktop Environment");
    SetVendorName(m_name + " Project");

    m_moduleRegistry = new ModuleRegistry(&app);

    if (!initializeModules()) {
        wxLogError("Failed to initialize modules");
        return false;
    }

    createUI();
    setupEventHandlers();
    ServiceManager::getInstance().startAllServices(*m_moduleRegistry);
    refreshDesktop();
    processStartFiles();

    return true;
}

int ShellApp::OnExit() {
    wxLogInfo("Shell exiting");
    return wxApp::OnExit();
}

ShellApp* ShellApp::getInstance() { return m_instance; }

void ShellApp::launchModule(ModulePtr module) {
    launchModule(module, {});
}

void ShellApp::launchModule(ModulePtr module, std::vector<std::string> args) {
    if (!module) {
        wxLogError("Cannot launch null module");
        return;
    }

    if (!module->isEnabled()) {
        wxLogWarning("Module %s is not enabled", module->getFullUri());
        wxMessageBox(_("Module is not enabled"), _("Error"), wxOK | wxICON_WARNING);
        return;
    }

    try {
        wxLogInfo("Launching module: %s", module->label);
        module->recordExecution();
        ProcessPtr p = module->run(app.makeRunConfig(std::move(args)));
        if (p && m_taskbar) {
            wxWindow* w = p->primaryWindow();
            if (w) {
                m_taskbar->addProcess(p);
            }
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to launch module %s: %s", module->getFullUri(), e.what());
        wxMessageBox(wxString::Format(_("Failed to launch %s: %s"), module->label, wxString(e.what())),
                     _("Error"), wxOK | wxICON_ERROR);
    }
}

void ShellApp::processStartFiles() {
    if (!m_volumeManager || !m_moduleRegistry || app.startFiles.empty())
        return;

    for (const std::string& spec : app.startFiles) {
        if (spec.rfind("vol://", 0) != 0)
            continue;

        Volume* anchor = m_volumeManager->getDefaultVolume();
        if (!anchor)
            continue;
        VolumeFile vf(anchor, "/");
        if (!parseVolUrl(m_volumeManager, spec, vf))
            continue;

        std::string path = vf.getPath();
        size_t slash = path.rfind('/');
        std::string ext;
        if (slash != std::string::npos && slash + 1 < path.size()) {
            const std::string base = path.substr(slash + 1);
            const size_t dot = base.rfind('.');
            if (dot != std::string::npos && dot + 1 < base.size()) {
                ext = base.substr(dot + 1);
                for (auto& c : ext) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
            }
        }

        const auto modUri = getFirstOpenWithModuleForExtension(ext);
        if (!modUri)
            continue;

        ModulePtr m = m_moduleRegistry->getOrCreateModule(*modUri);
        if (m && m->isEnabled())
            launchModule(m, {spec});
    }
}

void ShellApp::refreshDesktop() {
    if (m_desktop) {
        m_desktop->clearIcons();
        m_desktop->addVolumeIcons();
        m_desktop->loadBackgroundSettings();
        if (m_moduleRegistry) {
            for (auto& module : m_moduleRegistry->getVisibleModules()) {
                m_desktop->addIcon(module);
            }
        }
        m_desktop->arrangeIcons();
        m_desktop->loadLayout();
    }
}

bool ShellApp::initializeModules() {
    wxLogInfo("Initializing module system");

    // Install all registered modules
    if (m_moduleRegistry) {
        m_moduleRegistry->installAll();
    }

    return true;
}

void ShellApp::createUI() {
    wxLogInfo("Creating shell UI");

    // Create main frame
    m_mainWindow = new wxcFrame(nullptr, wxID_ANY, "OmniShell", //
                                wxDefaultPosition, wxSize(1024, 768));
    m_mainWindow->SetName(wxT("shell"));
    m_mainWindow->SetMinSize(wxSize(800, 600));
    m_mainWindow->SetBackgroundColour(*wxWHITE);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Layer 1: icons layer (desktop)
    m_desktop = new DesktopWindow(m_mainWindow);
    m_desktop->setVolumeManager(m_volumeManager);
    mainSizer->Add(m_desktop, 1, wxEXPAND);

    // Layer 2: control layer (separator + taskbar)
    wxPanel* controlPanel = new wxPanel(m_mainWindow);
    wxBoxSizer* controlSizer = new wxBoxSizer(wxVERTICAL);

    controlSizer->Add(new wxStaticLine(controlPanel, wxID_ANY), 0, wxEXPAND);

    m_taskbar = new Taskbar(controlPanel);
    controlSizer->Add(m_taskbar, 0, wxEXPAND);

    controlPanel->SetSizer(controlSizer);
    mainSizer->Add(controlPanel, 0, wxEXPAND);

    // Start menu as overlay (not sizer-managed) so it can expand without squeezing GTK widgets.
    m_startMenu = new StartMenu(m_mainWindow);
    m_startMenu->Hide();

    m_mainWindow->SetSizer(mainSizer);

    if (m_moduleRegistry) {
        m_startMenu->populateModules(m_moduleRegistry->getVisibleModules());
    }

    // Click outside start menu -> hide
    auto hideStartIfOutside = [this](wxWindow* source, wxMouseEvent& e) {
        if (!m_startMenu || !m_startMenu->IsShown()) {
            e.Skip();
            return;
        }
        wxPoint screenClick = source->ClientToScreen(e.GetPosition());
        if (!m_startMenu->ContainsScreenPoint(screenClick))
            m_startMenu->HideMenu();
        e.Skip();
    };

    m_mainWindow->Bind(wxEVT_LEFT_DOWN, [this, hideStartIfOutside](wxMouseEvent& e) {
        hideStartIfOutside(m_mainWindow, e);
    });
    if (m_desktop) {
        m_desktop->Bind(wxEVT_LEFT_DOWN, [this, hideStartIfOutside](wxMouseEvent& e) {
            hideStartIfOutside(m_desktop, e);
        });
        m_desktop->Bind(wxEVT_RIGHT_DOWN, [this, hideStartIfOutside](wxMouseEvent& e) {
            hideStartIfOutside(m_desktop, e);
        });
    }
    if (m_taskbar) {
        m_taskbar->Bind(wxEVT_LEFT_DOWN, [this, hideStartIfOutside](wxMouseEvent& e) {
            hideStartIfOutside(m_taskbar, e);
        });
        m_taskbar->Bind(wxEVT_RIGHT_DOWN, [this, hideStartIfOutside](wxMouseEvent& e) {
            hideStartIfOutside(m_taskbar, e);
        });
    }

    // When start menu is visible, route character input to it so typing opens the search box.
    m_mainWindow->Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
        bool forwardKey = true;
        if (m_startMenu && m_startMenu->IsShown() && m_startMenu->HandleGlobalKey(e))
            forwardKey = false;
        if (forwardKey)
            e.Skip();
    });

    m_mainWindow->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
        if (m_startMenu && m_startMenu->IsShown())
            positionStartMenu();
        e.Skip();
    });

    m_mainWindow->CenterOnScreen();
    m_mainWindow->Show(true);

    // Ensure the app quits when the main shell frame is closed.
    // Without this, wxWidgets may destroy the frame but keep the event loop running.
    if (m_mainWindow) {
        m_mainWindow->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) {
            e.Skip(); // Let the frame close normally.
            wxTheApp->CallAfter([]() {
                if (wxTheApp)
                    wxTheApp->ExitMainLoop();
            });
        });
    }

    wxLogInfo("Shell UI created");
}

void ShellApp::setupEventHandlers() {
    // Setup start menu launch callback
    if (m_startMenu) {
        m_startMenu->setLaunchCallback([this](ModulePtr module) { launchModule(module); });
    }
}

void ShellApp::openExplorerAt(const std::string& dir) {
    ModuleRegistry* mr = m_moduleRegistry;
    if (!mr || !m_volumeManager)
        return;

    Volume* vol = m_volumeManager->getDefaultVolume();
    if (!vol)
        return;

    // Ensure the directory exists; ignore errors, Explorer will report if it cannot open.
    const std::string path = "/" + dir;
    try {
        if (!vol->exists(path)) {
            vol->createDirectory(path);
        }
    } catch (...) {
    }

    // Open via Explorer application helper.
    ProcessPtr p = ExplorerApp::open(vol, path);
    if (p && m_taskbar) {
        m_taskbar->addProcess(p);
    }
}

void ShellApp::toggleStartMenu() {
    if (!m_startMenu || !m_mainWindow)
        return;
    if (m_startMenu->IsShown()) {
        m_startMenu->HideMenu();
        return;
    }
    m_startMenu->InvalidateBestSize();
    m_startMenu->Layout();
    positionStartMenu();
    m_startMenu->ShowMenu();
}

void ShellApp::positionStartMenu() {
    if (!m_startMenu || !m_mainWindow || !m_taskbar)
        return;

    const wxSize frameClient = m_mainWindow->GetClientSize();
    const int taskbarH = m_taskbar->GetSize().GetHeight() > 0 ? m_taskbar->GetSize().GetHeight()
                                                             : Taskbar::GetDefaultHeight();
    // Prefer the control's best (recommended) size rather than min size.
    m_startMenu->Layout();
    wxSize bestSize = m_startMenu->GetBestSize();
    int menuW = bestSize.GetWidth() > 0 ? bestSize.GetWidth() : m_startMenu->GetSize().GetWidth();
    if (menuW <= 0)
        menuW = 280;

    // Height: fit content, but do not exceed the area above the taskbar.
    const int maxH = std::max(0, frameClient.GetHeight() - taskbarH);
    if (maxH <= 0)
        return;
    int desiredH = bestSize.GetHeight() > 0 ? bestSize.GetHeight() : m_startMenu->GetSize().GetHeight();
    if (desiredH <= 0)
        desiredH = 420;
    int menuH = std::min(desiredH, maxH);

    // Width clamp: keep within client width for GTK stability.
    if (menuW > frameClient.GetWidth())
        menuW = frameClient.GetWidth();
    if (menuW < 60)
        menuW = 60;

    const int x = 0;
    int y = frameClient.GetHeight() - taskbarH - menuH;
    if (y < 0)
        y = 0;

    m_startMenu->SetSize(wxSize(menuW, menuH));
    m_startMenu->SetPosition(wxPoint(x, y));
    m_startMenu->Raise();
}

} // namespace os

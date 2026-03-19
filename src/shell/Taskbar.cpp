#include "Taskbar.hpp"

#include "Shell.hpp"

#include <bas/ui/arch/ImageSet.hpp>

#include <wx/datetime.h>
#include <wx/log.h>
#include <wx/toplevel.h>
#include <wx/uiaction.h>

#include <algorithm>

namespace os {

BEGIN_EVENT_TABLE(Taskbar, wxPanel)
    EVT_SIZE(Taskbar::OnSize)
    EVT_PAINT(Taskbar::OnPaint)
END_EVENT_TABLE()

Taskbar::Taskbar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
    , m_startMenu(nullptr)
    , m_buttonWidth(150)
    , m_buttonSpacing(2)
    , m_margin(0)
{
    SetMinSize(wxSize(-1, GetDefaultHeight()));
    SetMaxSize(wxSize(-1, GetDefaultHeight()));
    
    // Start button: white text, bold, larger font; no margin (flush to edges)
    int h = GetDefaultHeight();
    m_startButton = new wxButton(this, wxID_ANY, "Start",
                                 wxPoint(0, 0), wxSize(100, h));
    m_startButton->SetMinSize(wxSize(60, h));
    m_startButton->SetBackgroundColour(wxColour(0x0A, 0x24, 0x6A));
    m_startButton->SetForegroundColour(*wxWHITE);
    m_startButton->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    std::string iconDir = "streamline-vectors/core/pop/interface-essential";
    wxBitmap startBmp = ImageSet(Path(iconDir, "brightness-3.svg"))
                            .toBitmap1(24, 24);
    if (startBmp.IsOk()) {
        m_startButton->SetBitmap(startBmp, wxLEFT);
        m_startButton->SetBitmapMargins(4, 0);
    }
    m_startButton->Bind(wxEVT_BUTTON, &Taskbar::OnStartButtonClick, this);
    m_startButton->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent&) {
        m_startButton->SetForegroundColour(*wxWHITE);
    });
    
    m_appListPanel = new wxPanel(this, wxID_ANY, wxPoint(100, 0), wxSize(100, h));
    m_appListPanel->SetBackgroundColour(*wxWHITE);
    
    m_trayPanel = new wxPanel(this, wxID_ANY);
    m_trayPanel->SetBackgroundColour(*wxWHITE);
    
    m_clock = new wxStaticText(m_trayPanel, wxID_ANY, "",
                               wxPoint(5, 5), wxSize(100, 20),
                               wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL);
    m_clock->SetForegroundColour(*wxBLACK);
    
    // Start timer for clock
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
        m_clock->SetLabel(wxDateTime::Now().Format("%H:%M"));
    }, wxID_ANY);
    wxTimer* timer = new wxTimer(this);
    timer->Start(1000);
    m_clock->SetLabel(wxDateTime::Now().Format("%H:%M"));
    
    SetBackgroundColour(wxColour(200, 200, 200));
}

Taskbar::~Taskbar() {
}

void Taskbar::addApplication(ModulePtr module, wxWindow* window) {
    if (!module || !window) return;
    
    // Check if already in taskbar
    if (FindButtonByModule(module)) {
        return;
    }
    
    TaskbarButton button;
    button.module = module;
    button.window = window;
    button.active = false;
    button.minimized = false;
    
    // Create button
    button.button = new wxButton(m_appListPanel, wxID_ANY, module->label,
                                  wxDefaultPosition, wxSize(140, 28));
    button.button->Bind(wxEVT_BUTTON, &Taskbar::OnAppButtonClick, this);
    
    m_applications.push_back(button);
    UpdateButtonLayout();
}

void Taskbar::addProcess(ProcessPtr process) {
    if (!process)
        return;

    auto addOne = [this, process](wxWindow* window) {
        if (!window)
            return;
        if (FindButtonByWindow(window))
            return;

        TaskbarButton button;
        button.process = process;
        button.module = nullptr;
        button.window = window;
        button.active = false;
        button.minimized = false;

        button.button = new wxButton(m_appListPanel, wxID_ANY, process->label,
                                     wxDefaultPosition, wxSize(140, 28));
        wxBitmap bmp = process->icon.toBitmap1(16, 16);
        if (bmp.IsOk()) {
            button.button->SetBitmap(bmp, wxLEFT);
            button.button->SetBitmapMargins(4, 0);
        }
        button.button->Bind(wxEVT_BUTTON, &Taskbar::OnAppButtonClick, this);

        m_applications.push_back(button);
        SetButtonVisual(m_applications.back());

        // Sync taskbar state with window lifecycle.
        if (auto* tlw = dynamic_cast<wxTopLevelWindow*>(window)) {
            tlw->Bind(wxEVT_ACTIVATE, [this, window](wxActivateEvent& e) {
                for (auto& b : m_applications) {
                    if (b.window == window) {
                        b.active = e.GetActive();
                        SetButtonVisual(b);
                    } else if (e.GetActive()) {
                        b.active = false;
                        SetButtonVisual(b);
                    }
                }
                e.Skip();
            });
            tlw->Bind(wxEVT_ICONIZE, [this, window](wxIconizeEvent& e) {
                for (auto& b : m_applications) {
                    if (b.window == window) {
                        b.minimized = e.IsIconized();
                        SetButtonVisual(b);
                        break;
                    }
                }
                e.Skip();
            });
            tlw->Bind(wxEVT_CLOSE_WINDOW, [this, window](wxCloseEvent& e) {
                for (size_t i = 0; i < m_applications.size(); ++i) {
                    if (m_applications[i].window == window) {
                        RemoveButtonByIndex(i);
                        break;
                    }
                }
                e.Skip();
            });
        }
        UpdateButtonLayout();
    };

    // Subscribe to future window additions and add current windows.
    process->setOnWindowAdded([addOne](ProcessPtr, wxWindow* w) { addOne(w); });
    for (wxWindow* w : process->windows()) {
        addOne(w);
    }
}

void Taskbar::removeProcess(ProcessPtr process) {
    if (!process)
        return;
    for (size_t i = 0; i < m_applications.size(); ++i) {
        if (m_applications[i].process == process) {
            RemoveButtonByIndex(i);
            return;
        }
    }
}

void Taskbar::removeApplication(ModulePtr module) {
    auto it = std::find_if(m_applications.begin(), m_applications.end(),
        [&module](const TaskbarButton& tb) {
            return tb.module == module;
        });
    
    if (it != m_applications.end()) {
        if (it->button) {
            it->button->Destroy();
        }
        m_applications.erase(it);
        UpdateButtonLayout();
    }
}

void Taskbar::RemoveButtonByIndex(size_t idx) {
    if (idx >= m_applications.size())
        return;
    if (m_applications[idx].button)
        m_applications[idx].button->Destroy();
    m_applications.erase(m_applications.begin() + (long)idx);
    UpdateButtonLayout();
}

void Taskbar::updateApplicationState(ModulePtr module, bool active, bool minimized) {
    TaskbarButton* button = FindButtonByModule(module);
    if (button) {
        button->active = active;
        button->minimized = minimized;
        
        if (button->button) {
            if (active) {
                button->button->SetBackgroundColour(wxColour(100, 149, 237)); // Active color
            } else {
                button->button->SetBackgroundColour(*wxWHITE);
            }
            button->button->Refresh();
        }
    }
}

void Taskbar::SetButtonVisual(TaskbarButton& b) {
    if (!b.button)
        return;
    if (b.active) {
        b.button->SetBackgroundColour(wxColour(100, 149, 237));
    } else if (b.minimized) {
        b.button->SetBackgroundColour(wxColour(230, 230, 230));
    } else {
        b.button->SetBackgroundColour(*wxWHITE);
    }
    b.button->Refresh();
}

void Taskbar::setStartMenu(wxMenu* menu) {
    m_startMenu = menu;
}

void Taskbar::OnStartButtonClick(wxCommandEvent& event) {
    ShellApp* shell = ShellApp::getInstance();
    if (shell)
        shell->toggleStartMenu();
}

void Taskbar::OnAppButtonClick(wxCommandEvent& event) {
    // Find which button was clicked
    wxObject* obj = event.GetEventObject();
    
    for (auto& app : m_applications) {
        if (app.button == obj) {
            if (app.window) {
                if (app.minimized) {
                    // Restore window
                    app.window->Show(true);
                    app.window->Raise();
                    app.window->SetFocus();
                } else if (!app.active) {
                    // Raise window
                    app.window->Raise();
                    app.window->SetFocus();
                } else {
                    // Minimize window (hide for now)
                    app.window->Show(false);
                }
            }
            break;
        }
    }
    
    event.Skip();
}

void Taskbar::OnSize(wxSizeEvent& event) {
    wxSize size = GetClientSize();
    int h = size.GetHeight();
    
    m_startButton->SetPosition(wxPoint(0, 0));
    int startW = 100;
    if (size.GetWidth() > 0)
        startW = std::min(100, size.GetWidth());
    if (startW < 60)
        startW = std::max(1, startW);
    m_startButton->SetSize(wxSize(startW, h));
    
    m_appListPanel->SetPosition(wxPoint(startW, 0));
    int trayWidth = 160;
    if (trayWidth > size.GetWidth() - startW)
        trayWidth = std::max(0, size.GetWidth() - startW);
    int appListW = size.GetWidth() - startW - trayWidth;
    if (appListW < 0)
        appListW = 0;
    m_appListPanel->SetSize(wxSize(appListW, h));
    
    int trayX = startW + appListW;
    if (trayX < 0)
        trayX = 0;
    m_trayPanel->SetPosition(wxPoint(trayX, 0));
    m_trayPanel->SetSize(wxSize(size.GetWidth() - trayX, h));
    
    m_clock->SetPosition(wxPoint(230 - 105, 10));
    
    UpdateButtonLayout();
    event.Skip();
}

void Taskbar::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    event.Skip();
}

void Taskbar::UpdateButtonLayout() {
    int x = 0;
    int y = 0;
    const int panelW = m_appListPanel ? m_appListPanel->GetClientSize().GetWidth() : 0;
    for (auto& app : m_applications) {
        if (app.button) {
            app.button->SetPosition(wxPoint(x, y));
            int w = m_buttonWidth;
            if (panelW > 0)
                w = std::min(w, std::max(1, panelW - x));
            if (w < 60)
                w = std::max(1, w);
            app.button->SetMinSize(wxSize(60, 28));
            app.button->SetSize(wxSize(w, 28));
            x += m_buttonWidth + m_buttonSpacing;
        }
    }
    m_appListPanel->Layout();
}

TaskbarButton* Taskbar::FindButtonByModule(ModulePtr module) {
    for (auto& app : m_applications) {
        if (app.module == module) {
            return &app;
        }
    }
    return nullptr;
}

TaskbarButton* Taskbar::FindButtonByWindow(wxWindow* window) {
    for (auto& app : m_applications) {
        if (app.window == window) {
            return &app;
        }
    }
    return nullptr;
}

} // namespace os

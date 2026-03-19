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
    , startMenu_(nullptr)
    , buttonWidth_(150)
    , buttonSpacing_(2)
    , margin_(0)
{
    SetMinSize(wxSize(-1, GetDefaultHeight()));
    SetMaxSize(wxSize(-1, GetDefaultHeight()));
    
    // Start button: white text, bold, larger font; no margin (flush to edges)
    int h = GetDefaultHeight();
    startButton_ = new wxButton(this, wxID_ANY, "Start",
                                 wxPoint(0, 0), wxSize(100, h));
    startButton_->SetMinSize(wxSize(60, h));
    startButton_->SetBackgroundColour(wxColour(0x0A, 0x24, 0x6A));
    startButton_->SetForegroundColour(*wxWHITE);
    startButton_->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    std::string iconDir = "streamline-vectors/core/pop/interface-essential";
    wxBitmap startBmp = ImageSet(Path(iconDir, "brightness-3.svg"))
                            .toBitmap1(24, 24);
    if (startBmp.IsOk()) {
        startButton_->SetBitmap(startBmp, wxLEFT);
        startButton_->SetBitmapMargins(4, 0);
    }
    startButton_->Bind(wxEVT_BUTTON, &Taskbar::OnStartButtonClick, this);
    startButton_->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent&) {
        startButton_->SetForegroundColour(*wxWHITE);
    });
    
    appListPanel_ = new wxPanel(this, wxID_ANY, wxPoint(100, 0), wxSize(100, h));
    appListPanel_->SetBackgroundColour(*wxWHITE);
    
    trayPanel_ = new wxPanel(this, wxID_ANY);
    trayPanel_->SetBackgroundColour(*wxWHITE);
    
    clock_ = new wxStaticText(trayPanel_, wxID_ANY, "",
                               wxPoint(5, 5), wxSize(100, 20),
                               wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL);
    clock_->SetForegroundColour(*wxBLACK);
    
    // Start timer for clock
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
        clock_->SetLabel(wxDateTime::Now().Format("%H:%M"));
    }, wxID_ANY);
    wxTimer* timer = new wxTimer(this);
    timer->Start(1000);
    clock_->SetLabel(wxDateTime::Now().Format("%H:%M"));
    
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
    button.button = new wxButton(appListPanel_, wxID_ANY, module->label,
                                  wxDefaultPosition, wxSize(140, 28));
    button.button->Bind(wxEVT_BUTTON, &Taskbar::OnAppButtonClick, this);
    
    applications_.push_back(button);
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

        button.button = new wxButton(appListPanel_, wxID_ANY, process->label,
                                     wxDefaultPosition, wxSize(140, 28));
        wxBitmap bmp = process->icon.toBitmap1(16, 16);
        if (bmp.IsOk()) {
            button.button->SetBitmap(bmp, wxLEFT);
            button.button->SetBitmapMargins(4, 0);
        }
        button.button->Bind(wxEVT_BUTTON, &Taskbar::OnAppButtonClick, this);

        applications_.push_back(button);
        SetButtonVisual(applications_.back());

        // Sync taskbar state with window lifecycle.
        if (auto* tlw = dynamic_cast<wxTopLevelWindow*>(window)) {
            tlw->Bind(wxEVT_ACTIVATE, [this, window](wxActivateEvent& e) {
                for (auto& b : applications_) {
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
                for (auto& b : applications_) {
                    if (b.window == window) {
                        b.minimized = e.IsIconized();
                        SetButtonVisual(b);
                        break;
                    }
                }
                e.Skip();
            });
            tlw->Bind(wxEVT_CLOSE_WINDOW, [this, window](wxCloseEvent& e) {
                for (size_t i = 0; i < applications_.size(); ++i) {
                    if (applications_[i].window == window) {
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
    for (size_t i = 0; i < applications_.size(); ++i) {
        if (applications_[i].process == process) {
            RemoveButtonByIndex(i);
            return;
        }
    }
}

void Taskbar::removeApplication(ModulePtr module) {
    auto it = std::find_if(applications_.begin(), applications_.end(),
        [&module](const TaskbarButton& tb) {
            return tb.module == module;
        });
    
    if (it != applications_.end()) {
        if (it->button) {
            it->button->Destroy();
        }
        applications_.erase(it);
        UpdateButtonLayout();
    }
}

void Taskbar::RemoveButtonByIndex(size_t idx) {
    if (idx >= applications_.size())
        return;
    if (applications_[idx].button)
        applications_[idx].button->Destroy();
    applications_.erase(applications_.begin() + (long)idx);
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
    startMenu_ = menu;
}

void Taskbar::OnStartButtonClick(wxCommandEvent& event) {
    ShellApp* shell = ShellApp::getInstance();
    if (shell)
        shell->toggleStartMenu();
}

void Taskbar::OnAppButtonClick(wxCommandEvent& event) {
    // Find which button was clicked
    wxObject* obj = event.GetEventObject();
    
    for (auto& app : applications_) {
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
    
    startButton_->SetPosition(wxPoint(0, 0));
    int startW = 100;
    if (size.GetWidth() > 0)
        startW = std::min(100, size.GetWidth());
    if (startW < 60)
        startW = std::max(1, startW);
    startButton_->SetSize(wxSize(startW, h));
    
    appListPanel_->SetPosition(wxPoint(startW, 0));
    int trayWidth = 160;
    if (trayWidth > size.GetWidth() - startW)
        trayWidth = std::max(0, size.GetWidth() - startW);
    int appListW = size.GetWidth() - startW - trayWidth;
    if (appListW < 0)
        appListW = 0;
    appListPanel_->SetSize(wxSize(appListW, h));
    
    int trayX = startW + appListW;
    if (trayX < 0)
        trayX = 0;
    trayPanel_->SetPosition(wxPoint(trayX, 0));
    trayPanel_->SetSize(wxSize(size.GetWidth() - trayX, h));
    
    clock_->SetPosition(wxPoint(230 - 105, 10));
    
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
    const int panelW = appListPanel_ ? appListPanel_->GetClientSize().GetWidth() : 0;
    for (auto& app : applications_) {
        if (app.button) {
            app.button->SetPosition(wxPoint(x, y));
            int w = buttonWidth_;
            if (panelW > 0)
                w = std::min(w, std::max(1, panelW - x));
            if (w < 60)
                w = std::max(1, w);
            app.button->SetMinSize(wxSize(60, 28));
            app.button->SetSize(wxSize(w, 28));
            x += buttonWidth_ + buttonSpacing_;
        }
    }
    appListPanel_->Layout();
}

TaskbarButton* Taskbar::FindButtonByModule(ModulePtr module) {
    for (auto& app : applications_) {
        if (app.module == module) {
            return &app;
        }
    }
    return nullptr;
}

TaskbarButton* Taskbar::FindButtonByWindow(wxWindow* window) {
    for (auto& app : applications_) {
        if (app.window == window) {
            return &app;
        }
    }
    return nullptr;
}

} // namespace os

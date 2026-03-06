#include "Taskbar.hpp"

#include <wx/datetime.h>
#include <wx/log.h>

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
    , margin_(4)
{
    SetMinSize(wxSize(-1, GetDefaultHeight()));
    SetMaxSize(wxSize(-1, GetDefaultHeight()));
    
    // Create start button
    startButton_ = new wxButton(this, wxID_ANY, "Start",
                                 wxPoint(margin_, margin_),
                                 wxSize(80, GetDefaultHeight() - margin_ * 2));
    startButton_->Bind(wxEVT_BUTTON, &Taskbar::OnStartButtonClick, this);
    
    // Create application list panel
    appListPanel_ = new wxPanel(this, wxID_ANY,
                                 wxPoint(90, margin_),
                                 wxSize(100, GetDefaultHeight() - margin_ * 2));
    appListPanel_->SetBackgroundColour(*wxWHITE);
    
    // Create tray panel
    trayPanel_ = new wxPanel(this, wxID_ANY);
    trayPanel_->SetBackgroundColour(*wxWHITE);
    
    // Create clock
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

void Taskbar::setStartMenu(wxMenu* menu) {
    startMenu_ = menu;
}

void Taskbar::OnStartButtonClick(wxCommandEvent& event) {
    if (startMenu_) {
        // Position menu above taskbar - use fixed size for wx 3.0 compatibility
        wxPoint pos = startButton_->ClientToScreen(wxPoint(0, -400));
        PopupMenu(startMenu_, pos);
    }
    event.Skip();
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
    
    // Position start button
    startButton_->SetPosition(wxPoint(margin_, margin_));
    startButton_->SetSize(wxSize(80, size.GetHeight() - margin_ * 2));
    
    // Position app list panel
    appListPanel_->SetPosition(wxPoint(90, margin_));
    appListPanel_->SetSize(wxSize(size.GetWidth() - 250, size.GetHeight() - margin_ * 2));
    
    // Position tray panel
    trayPanel_->SetPosition(wxPoint(size.GetWidth() - 240, margin_));
    trayPanel_->SetSize(wxSize(230, size.GetHeight() - margin_ * 2));
    
    // Position clock
    clock_->SetPosition(wxPoint(230 - 105, 10));
    
    UpdateButtonLayout();
    event.Skip();
}

void Taskbar::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    event.Skip();
}

void Taskbar::UpdateButtonLayout() {
    int x = margin_;
    int y = margin_;
    
    for (auto& app : applications_) {
        if (app.button) {
            app.button->SetPosition(wxPoint(x, y));
            app.button->SetSize(wxSize(buttonWidth_, 28));
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

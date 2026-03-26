#include "StartMenu.hpp"

#include "Shell.hpp"

#include <bas/ui/arch/ImageSet.hpp>
#include "../core/App.hpp"

#include <wx/image.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/utils.h>

#include <algorithm>
#include <set>

#include "../ui/ThemeStyles.hpp"
#include "../wx/wxcWindow.hpp"
using namespace ThemeStyles;

namespace os {

// XP-style colors
static const wxColour kMenuBg(0xf0, 0xf0, 0xf0);
static const wxColour kItemHover(0x0a, 0x24, 0x6a);
static const wxColour kItemHoverText(0xff, 0xff, 0xff);
static const int kItemHeight = 28;
static const int kIconSize = 24;
static const int kMenuWidth = 280;
static const int kMenuHeight = 420;
static const int kSubMenuWidth = 260;
static const int kSubMenuMaxHeight = 420;

static wxBitmap ChevronBitmap(bool hover) {
    static wxBitmap cachedBlack;
    static wxBitmap cachedWhite;
    if (hover && cachedWhite.IsOk())
        return cachedWhite;
    if (!hover && cachedBlack.IsOk())
        return cachedBlack;

    ImageSet set = os::app.getIconTheme()->icon("shell", "startmenu.chevron");
    wxBitmap base = set.toBitmap1(12, 12);
    if (!base.IsOk()) {
        return base;
    }

    wxImage img = base.ConvertToImage();
    if (!img.IsOk()) {
        return base;
    }
    if (!img.HasAlpha()) {
        img.InitAlpha();
    }

    const wxColour target = hover ? *wxWHITE : *wxBLACK;
    const int w = img.GetWidth();
    const int h = img.GetHeight();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (img.GetAlpha(x, y) == 0)
                continue;
            img.SetRGB(x, y, target.Red(), target.Green(), target.Blue());
        }
    }
    wxBitmap tinted(img);
    if (hover)
        cachedWhite = tinted;
    else
        cachedBlack = tinted;
    return tinted;
}

StartMenu::StartMenu(wxWindow* parent)
    : wxcPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(kMenuWidth, kMenuHeight)),
      m_searchBox(nullptr), m_scrollArea(nullptr), m_categoryPanel(nullptr),
      m_launchCallback(nullptr), m_activeCategoryId(ID_CATEGORY_NONE), m_subMenu(nullptr),
      m_subScrollArea(nullptr), m_subSizer(nullptr), m_subMenuKind(ROW_LEAF),
      m_subMenuCategoryId(ID_CATEGORY_NONE), m_menuWidth(kMenuWidth), m_menuHeight(kMenuHeight) {
    SetBackgroundColour(kMenuBg);
    SetMinSize(wxSize(m_menuWidth, 200));
    SetName(wxT("start_menu"));

    // Root layout: left branding strip + right content with padding.
    wxBoxSizer* rootSizer = new wxBoxSizer(wxHORIZONTAL);

    wxPanel* branding = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(40, -1));
    branding->SetBackgroundColour(kItemHover);
    branding->SetName(wxT("branding"));
    wxcBind(*branding, wxEVT_PAINT, [](wxPaintEvent& e) {
        wxWindow* w = static_cast<wxWindow*>(e.GetEventObject());
        wxPaintDC dc(w);
        wxRect rect = w->GetClientRect();
        // Blue -> light gray gradient (left to right)
        dc.GradientFillLinear(rect, kItemHover, kMenuBg, wxNORTH);
        dc.SetTextForeground(*wxWHITE);
        wxFont font = w->GetFont();
        font.SetWeight(wxFONTWEIGHT_BOLD);
        dc.SetFont(font);
        wxString text = "OmniShell";
        if (auto* shell = ShellApp::getInstance()) {
            text = wxString(shell->getName());
        }
        wxSize txt = dc.GetTextExtent(text);
        wxSize sz = w->GetClientSize();
        // Draw rotated 90 degrees, roughly centered vertically, close to bottom edge.
        int x = (sz.GetWidth() - txt.GetHeight()) / 2;
        int y = sz.GetHeight() - 8;
        dc.DrawRotatedText(text, x, y, 90);
    });

    wxPanel* contentPanel = new wxPanel(this, wxID_ANY);
    contentPanel->SetBackgroundColour(kMenuBg);
    contentPanel->SetName(wxT("content"));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    // Always keep a top padding even when search box is hidden.
    mainSizer->AddSpacer(10);

    m_searchBox = new wxTextCtrl(contentPanel, wxID_ANY, "Search programs...", wxDefaultPosition,
                                 wxSize(-1, 28), wxTE_PROCESS_ENTER);
    m_searchBox->SetName(wxT("search"));
    wxcBind(*m_searchBox, wxEVT_TEXT, &StartMenu::OnSearch, this);
    wxcBind(*m_searchBox, wxEVT_TEXT_ENTER, [this](wxCommandEvent&) {
        if (!m_filteredModules.empty() && m_launchCallback) {
            m_launchCallback(m_filteredModules[0]);
        }
        HideMenu();
    });
    wxcBind(*m_searchBox, wxEVT_KEY_DOWN, &StartMenu::OnKeyDown, this);
    mainSizer->Add(m_searchBox, 0, wxEXPAND | wxALL, 10);
    // Start hidden; it will appear on first key press via HandleGlobalKey.
    m_searchBox->Show(false);

    m_scrollArea =
        new wxPanel(contentPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_scrollArea->SetBackgroundColour(kMenuBg);
    m_scrollArea->SetName(wxT("program_list"));
    // Stretch the list area so the category strip stays pinned to the bottom.
    // (The empty space, if any, lives inside the list area.)
    mainSizer->Add(m_scrollArea, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

    m_categoryPanel = new wxPanel(contentPanel, wxID_ANY);
    m_categoryPanel->SetName(wxT("category_bar"));
    wxBoxSizer* catSizer = new wxBoxSizer(wxHORIZONTAL);
    // Top category strip: icon-only buttons with tooltips.
    wxButton* allBtn = new wxButton(m_categoryPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                    wxBORDER_NONE);
    allBtn->SetMinSize(wxSize(44, -1));
    {
        // Use a known-good icon so ALL always has a bitmap.
        wxBitmap allBmp = os::app.getIconTheme()->icon("shell", "startmenu.all").toBitmap1(16, 16);
        if (allBmp.IsOk()) {
            allBtn->SetBitmap(allBmp, wxLEFT);
            allBtn->SetBitmapMargins(4, 0);
        }
    }
    allBtn->SetToolTip("All programs");
    allBtn->SetName(wxT("category_all"));
    wxcBind(*allBtn, wxEVT_BUTTON, [this](wxCommandEvent&) {
        m_activeCategoryId = ID_CATEGORY_NONE;
        wxString t = m_searchBox ? m_searchBox->GetValue() : "";
        FilterModules(t.ToStdString());
        CreateMenuContent();
    });
    wxcBind(*allBtn, wxEVT_ENTER_WINDOW, [allBtn](wxMouseEvent& e) {
        allBtn->SetBackgroundColour(wxColour(0xd0, 0xd0, 0xd0));
        allBtn->Refresh();
        e.Skip();
    });
    wxcBind(*allBtn, wxEVT_LEAVE_WINDOW, [allBtn](wxMouseEvent& e) {
        allBtn->SetBackgroundColour(wxNullColour);
        allBtn->Refresh();
        e.Skip();
    });
    catSizer->Add(allBtn, 1, wxEXPAND | wxALL, 2);
    for (const auto& cat : getAllCategories()) {
        wxButton* btn = new wxButton(m_categoryPanel, wxID_ANY, "", wxDefaultPosition,
                                     wxDefaultSize, wxBORDER_NONE);
        btn->SetMinSize(wxSize(44, -1));
        // Show the same icon as used for category rows in the main list, but scaled down a bit.
        auto catBmp = cat.icon.toBitmap1(16, 16);
        if (catBmp.IsOk()) {
            btn->SetBitmap(catBmp, wxLEFT);
            btn->SetBitmapMargins(4, 0);
        }
        btn->SetToolTip(cat.label);
        CategoryId id = cat.id;
        btn->SetName(wxString::Format(wxT("category_%d"), static_cast<int>(id)));
        wxcBind(*btn, wxEVT_BUTTON, [this, id](wxCommandEvent&) {
            m_activeCategoryId = id;
            wxString t = m_searchBox ? m_searchBox->GetValue() : "";
            FilterModules(t.ToStdString());
            CreateMenuContent();
        });
        wxcBind(*btn, wxEVT_ENTER_WINDOW,
                [btn](wxMouseEvent& e) {
                    btn->SetBackgroundColour(wxColour(0xd0, 0xd0, 0xd0));
                    btn->Refresh();
                    e.Skip();
                });
        wxcBind(*btn, wxEVT_LEAVE_WINDOW,
                [btn](wxMouseEvent& e) {
                    btn->SetBackgroundColour(wxNullColour);
                    btn->Refresh();
                    e.Skip();
                });
        catSizer->Add(btn, 1, wxEXPAND | wxALL, 2);
    }
    m_categoryPanel->SetSizer(catSizer);
    // Keep the category strip close to the bottom edge.
    mainSizer->Add(m_categoryPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    contentPanel->SetSizer(mainSizer);

    rootSizer->Add(branding, 0, wxEXPAND);
    rootSizer->Add(contentPanel, 1, wxEXPAND);

    SetSizer(rootSizer);

    Bind(wxEVT_LEAVE_WINDOW, &StartMenu::OnLeaveWindow, this);

    // Right-side submenu overlay (sibling overlay, like the start menu itself).
    m_subMenu = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(kSubMenuWidth, 200),
                            wxBORDER_SIMPLE);
    m_subMenu->SetBackgroundColour(kMenuBg);
    m_subMenu->SetName(wxT("submenu"));
    m_subMenu->Hide();
    wxcBind(*m_subMenu, wxEVT_LEAVE_WINDOW,
            &StartMenu::OnSubMenuLeaveWindow, this);
    m_subScrollArea =
        new wxPanel(m_subMenu, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_subScrollArea->SetBackgroundColour(kMenuBg);
    m_subScrollArea->SetName(wxT("submenu_list"));
    wxBoxSizer* subRoot = new wxBoxSizer(wxVERTICAL);
    subRoot->Add(m_subScrollArea, 1, wxEXPAND | wxALL, 4);
    m_subMenu->SetSizer(subRoot);
}

StartMenu::~StartMenu() {}

void StartMenu::ShowMenu() {
    Show(true);
    Raise();
    HideSubMenu();
    if (m_searchBox) {
        m_searchBox->Clear();
        m_searchBox->Show(false);
        InvalidateBestSize();
        Layout();
    }
}

void StartMenu::HideMenu() {
    Show(false);
    HideSubMenu();
    if (m_searchBox) {
        m_searchBox->Clear();
        m_searchBox->Show(false);
        InvalidateBestSize();
    }
}

bool StartMenu::ContainsScreenPoint(const wxPoint& screenPt) const {
    if (IsShown() && GetScreenRect().Contains(screenPt))
        return true;
    if (m_subMenu && m_subMenu->IsShown() && m_subMenu->GetScreenRect().Contains(screenPt))
        return true;
    return false;
}

void StartMenu::populateModules(const std::vector<ModulePtr>& modules) {
    m_allModules = modules;
    m_filteredModules = modules;
    CreateMenuContent();
}

void StartMenu::clearModules() {
    m_allModules.clear();
    m_filteredModules.clear();
    m_menuRows.clear();
    if (m_scrollArea) {
        m_scrollArea->DestroyChildren();
        m_scrollArea->GetSizer()->Clear(true);
    }
}

void StartMenu::setLaunchCallback(LaunchCallback callback) {
    if (!callback) {
        m_launchCallback = nullptr;
        return;
    }
    m_launchCallback = [this, cb = std::move(callback)](ModulePtr m) {
        wxInvokeChecked(this, nullptr, [&] { cb(std::move(m)); });
    };
}

void StartMenu::OnSearch(wxCommandEvent& event) {
    FilterModules(m_searchBox ? m_searchBox->GetValue().ToStdString() : "");
    CreateMenuContent();
    event.Skip();
}

void StartMenu::OnKeyDown(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_ESCAPE) {
        HideMenu();
        return;
    }
    event.Skip();
}

bool StartMenu::HandleGlobalKey(wxKeyEvent& event) {
    const int keyCode = event.GetKeyCode();

    // Esc closes the menu.
    if (keyCode == WXK_ESCAPE) {
        HideMenu();
        return true;
    }

    // Ignore pure modifier keys.
    if (event.HasModifiers() && !(event.ControlDown() && !event.AltDown() && !event.ShiftDown())) {
        return false;
    }

    // Determine the character typed.
    wxChar ch = 0;
    long uni = event.GetUnicodeKey();
    if (uni != WXK_NONE && uni >= 32) {
        ch = static_cast<wxChar>(uni);
    } else if (keyCode >= 32 && keyCode < 127) {
        ch = static_cast<wxChar>(keyCode);
    }

    if (!ch)
        return false;

    if (!m_searchBox)
        return false;

    // First key press: show the search box and start the query with that character.
    if (!m_searchBox->IsShown()) {
        m_searchBox->Show(true);
        InvalidateBestSize();
        Layout();
        wxString initial;
        initial.Append(ch);
        m_searchBox->ChangeValue(initial);
        m_searchBox->SetInsertionPointEnd();
        FilterModules(initial.ToStdString());
        CreateMenuContent();
        m_searchBox->SetFocus();
        return true;
    }

    // If already visible, let the normal text control handle it.
    return false;
}

void StartMenu::OnLeaveWindow(wxMouseEvent& event) { event.Skip(); }

void StartMenu::OnSubMenuLeaveWindow(wxMouseEvent& event) { event.Skip(); }

void StartMenu::FilterModules(const std::string& searchText) {
    m_filteredModules.clear();
    std::string lowerSearch = searchText;
    std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

    for (const auto& module : m_allModules) {
        if (!module->isVisible())
            continue;
        if (m_activeCategoryId != ID_CATEGORY_NONE && module->categoryId != m_activeCategoryId)
            continue;
        std::string name = module->name, label = module->label, desc = module->description;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::transform(label.begin(), label.end(), label.begin(), ::tolower);
        std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);
        if (name.find(lowerSearch) != std::string::npos ||
            label.find(lowerSearch) != std::string::npos ||
            desc.find(lowerSearch) != std::string::npos) {
            m_filteredModules.push_back(module);
        }
    }
}

void StartMenu::AddMenuItem(wxWindow* parent, wxSizer* sizer, const wxString& label,
                            ModulePtr module, bool isSeparator, const wxBitmap* iconBitmap,
                            RowKind rowKind, CategoryId categoryId) {
    if (isSeparator) {
        sizer->Add(new wxStaticLine(parent, wxID_ANY), 0, wxEXPAND | wxTOP | wxBOTTOM, 4);
        return;
    }
    wxPanel* row = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, kItemHeight));
    row->SetBackgroundColour(kMenuBg);
    row->SetMinSize(wxSize(-1, kItemHeight));

    wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
    if (iconBitmap && iconBitmap->IsOk()) {
        wxStaticBitmap* icon = new wxStaticBitmap(row, wxID_ANY, *iconBitmap);
        icon->SetBackgroundColour(kMenuBg);
        rowSizer->Add(icon, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    }
    wxStaticText* text = new wxStaticText(row, wxID_ANY, label, wxDefaultPosition, wxDefaultSize,
                                          wxALIGN_LEFT | wxST_ELLIPSIZE_END);
    text->SetBackgroundColour(kMenuBg);
    rowSizer->Add(text, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);

    // Right-aligned chevron indicator.
    const bool showChevron = (rowKind == ROW_CATEGORY_FOLDER || rowKind == ROW_RECENT_FOLDER);
    if (showChevron) {
        wxStaticBitmap* chev = new wxStaticBitmap(row, wxID_ANY, ChevronBitmap(false));
        chev->SetName("chevron");
        chev->SetBackgroundColour(kMenuBg);
        rowSizer->Add(chev, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    }
    row->SetSizer(rowSizer);
    row->SetName(wxT("row"));
    text->SetName(wxT("label"));

    m_menuRows.push_back({row, module, rowKind, categoryId});

    wxcBind(*row, wxEVT_LEFT_UP, &StartMenu::OnMenuItemClick, this);
    wxcBind(*row, wxEVT_ENTER_WINDOW, &StartMenu::OnMenuItemEnter, this);
    wxcBind(*row, wxEVT_LEAVE_WINDOW, &StartMenu::OnMenuItemLeave, this);
    wxcBind(*text, wxEVT_LEFT_UP, &StartMenu::OnMenuItemClick, this);
    wxcBind(*text, wxEVT_ENTER_WINDOW, &StartMenu::OnMenuItemEnter,
            this);
    wxcBind(*text, wxEVT_LEAVE_WINDOW, &StartMenu::OnMenuItemLeave,
            this);
    for (wxWindowList::const_iterator it = row->GetChildren().begin();
         it != row->GetChildren().end(); ++it) {
        wxcBind(**it, wxEVT_LEFT_UP, &StartMenu::OnMenuItemClick,
                this);
        wxcBind(**it, wxEVT_ENTER_WINDOW,
                &StartMenu::OnMenuItemEnter, this);
        wxcBind(**it, wxEVT_LEAVE_WINDOW,
                &StartMenu::OnMenuItemLeave, this);
    }

    sizer->Add(row, 0, wxEXPAND);
}

void StartMenu::OnMenuItemClick(wxMouseEvent& event) {
    wxWindow* win = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (!win)
        return;
    wxWindow* row = win;
    while (row && (row->GetParent() != m_scrollArea && row->GetParent() != m_subScrollArea))
        row = row->GetParent();
    if (!row)
        return;

    // If this row belongs to the right-hand submenu, map it to the submenu's module list.
    if (row->GetParent() == m_subScrollArea && m_subScrollArea) {
        wxSizer* s = m_subScrollArea->GetSizer();
        if (s) {
            const size_t count = s->GetItemCount();
            for (size_t i = 0; i < count; ++i) {
                wxSizerItem* item = s->GetItem(i);
                if (!item)
                    continue;
                if (item->GetWindow() == row) {
                    if (i < m_subMenuModules.size()) {
                        ModulePtr m = m_subMenuModules[i];
                        if (m && m_launchCallback) {
                            m_launchCallback(m);
                            HideMenu();
                        }
                    }
                    return;
                }
            }
        }
    }

    for (const auto& mr : m_menuRows) {
        if (mr.row != row)
            continue;
        if (mr.kind == ROW_CATEGORY_FOLDER || mr.kind == ROW_RECENT_FOLDER) {
            ShowSubMenuForRow(row);
            return;
        }
        if (mr.module) {
            if (m_launchCallback)
                m_launchCallback(mr.module);
            HideMenu();
            return;
        }
        wxString lbl;
        for (wxWindowList::const_iterator it = row->GetChildren().begin();
             it != row->GetChildren().end(); ++it) {
            wxStaticText* st = dynamic_cast<wxStaticText*>(*it);
            if (st) {
                lbl = st->GetLabel();
                break;
            }
        }
        if (lbl.StartsWith("Exit")) {
            if (wxTheApp)
                wxTheApp->ExitMainLoop();
        } else if (lbl.StartsWith("Trash")) {
            ShellApp* shell = ShellApp::getInstance();
            if (shell) {
                shell->openExplorerAt("Trash");
            }
        }
        HideMenu();
        return;
    }
}

void StartMenu::OnMenuItemEnter(wxMouseEvent& event) {
    wxWindow* w = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (!w)
        return;
    wxWindow* row = w;
    while (row && (row->GetParent() != m_scrollArea && row->GetParent() != m_subScrollArea))
        row = row->GetParent();
    if (row) {
        row->SetBackgroundColour(kItemHover);
        for (wxWindowList::const_iterator it = row->GetChildren().begin();
             it != row->GetChildren().end(); ++it) {
            (*it)->SetBackgroundColour(kItemHover);
            wxStaticText* st = dynamic_cast<wxStaticText*>(*it);
            if (st)
                st->SetForegroundColour(kItemHoverText);
            wxStaticBitmap* sb = dynamic_cast<wxStaticBitmap*>(*it);
            if (sb && sb->GetName() == "chevron")
                sb->SetBitmap(ChevronBitmap(true));
        }
        row->Refresh();
    }
    if (row) {
        for (const auto& mr : m_menuRows) {
            if (mr.row == row && (mr.kind == ROW_CATEGORY_FOLDER || mr.kind == ROW_RECENT_FOLDER)) {
                ShowSubMenuForRow(row);
                break;
            }
        }
    }
    event.Skip();
}

void StartMenu::OnMenuItemLeave(wxMouseEvent& event) {
    wxWindow* w = dynamic_cast<wxWindow*>(event.GetEventObject());
    if (!w)
        return;
    wxWindow* row = w;
    while (row && (row->GetParent() != m_scrollArea && row->GetParent() != m_subScrollArea))
        row = row->GetParent();
    if (row) {
        row->SetBackgroundColour(kMenuBg);
        for (wxWindowList::const_iterator it = row->GetChildren().begin();
             it != row->GetChildren().end(); ++it) {
            (*it)->SetBackgroundColour(kMenuBg);
            wxStaticText* st = dynamic_cast<wxStaticText*>(*it);
            if (st)
                st->SetForegroundColour(*wxBLACK);
            wxStaticBitmap* sb = dynamic_cast<wxStaticBitmap*>(*it);
            if (sb && sb->GetName() == "chevron")
                sb->SetBitmap(ChevronBitmap(false));
        }
        row->Refresh();
    }
    event.Skip();
}

void StartMenu::ShowSubMenuForRow(wxWindow* row) {
    if (!row || !m_subMenu || !m_subScrollArea)
        return;
    for (const auto& mr : m_menuRows) {
        if (mr.row != row)
            continue;
        if (mr.kind != ROW_CATEGORY_FOLDER && mr.kind != ROW_RECENT_FOLDER)
            return;
        if (m_subMenu->IsShown() && m_subMenuKind == mr.kind &&
            m_subMenuCategoryId == mr.categoryId) {
            PositionSubMenuNearRow(row);
            return;
        }
        BuildSubMenuContent(mr.kind, mr.categoryId);
        PositionSubMenuNearRow(row);
        m_subMenu->Show(true);
        m_subMenu->Raise();
        return;
    }
}

void StartMenu::HideSubMenu() {
    if (m_subMenu)
        m_subMenu->Show(false);
    m_subMenuKind = ROW_LEAF;
    m_subMenuCategoryId = ID_CATEGORY_NONE;
}

void StartMenu::BuildSubMenuContent(RowKind kind, CategoryId categoryId) {
    if (!m_subScrollArea)
        return;

    m_subScrollArea->DestroyChildren();
    m_subMenuKind = kind;
    m_subMenuCategoryId = categoryId;
    m_subMenuModules.clear();

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    auto addLeaf = [&](ModulePtr m) {
        if (!m)
            return;
        auto b = m->image.toBitmap1(kIconSize, kIconSize);
        AddMenuItem(m_subScrollArea, sizer, m->label, m, false, b.IsOk() ? &b : nullptr, ROW_LEAF,
                    ID_CATEGORY_NONE);
        m_subMenuModules.push_back(m);
    };

    if (kind == ROW_CATEGORY_FOLDER) {
        for (const auto& m : m_filteredModules) {
            if (m && m->categoryId == categoryId)
                addLeaf(m);
        }
        if (sizer->IsEmpty()) {
            AddMenuItem(m_subScrollArea, sizer, "  (empty)", nullptr, false, nullptr, ROW_LEAF,
                        ID_CATEGORY_NONE);
            m_subMenuModules.push_back(nullptr);
        }
    } else if (kind == ROW_RECENT_FOLDER) {
        // Real recent list: show most recently run visible modules.
        std::vector<ModulePtr> mods = m_allModules;
        mods.erase(std::remove_if(mods.begin(), mods.end(),
                                  [](const ModulePtr& m) {
                                      return !m || !m->isVisible() || !m->isEnabled() ||
                                             !m->lastRunTime.IsValid();
                                  }),
                   mods.end());
        std::sort(mods.begin(), mods.end(), [](const ModulePtr& a, const ModulePtr& b) {
            return a->lastRunTime.IsLaterThan(b->lastRunTime);
        });

        const size_t kMax = 8;
        size_t shown = 0;
        for (const auto& m : mods) {
            if (shown >= kMax)
                break;
            addLeaf(m);
            ++shown;
        }
        if (shown == 0) {
            AddMenuItem(m_subScrollArea, sizer, "  (no recent items)", nullptr, false, nullptr,
                        ROW_LEAF, ID_CATEGORY_NONE);
            m_subMenuModules.push_back(nullptr);
        }
    }

    m_subScrollArea->SetSizer(sizer);
    m_subScrollArea->Layout();
    m_subSizer = sizer;
}

void StartMenu::PositionSubMenuNearRow(wxWindow* row) {
    if (!row || !m_subMenu || !GetParent())
        return;

    wxRect menuRect = GetScreenRect();
    wxRect rowRect = row->GetScreenRect();

    wxWindow* top = GetParent();
    wxPoint menuClient = top->ScreenToClient(wxPoint(menuRect.x, menuRect.y));
    wxPoint rowClient = top->ScreenToClient(wxPoint(rowRect.x, rowRect.y));

    const wxSize topClient = top->GetClientSize();
    int w = kSubMenuWidth;

    // Fit submenu height to content, but clamp so it stays usable.
    int contentH = 0;
    if (m_subSizer) {
        // Add a small padding to avoid tight clipping on some themes.
        contentH = m_subSizer->GetMinSize().GetHeight() + 8;
    }
    int h = contentH > 0 ? contentH : 200;
    h = std::min(h, std::min(kSubMenuMaxHeight, topClient.GetHeight()));
    if (h < 60)
        h = 60;

    // Prefer opening to the right; if no space, open to the left of the start menu.
    int x = menuClient.x + menuRect.width - 1;
    if (x + w > topClient.GetWidth()) {
        x = menuClient.x - w + 1;
    }
    if (x < 0)
        x = 0;
    if (x + w > topClient.GetWidth())
        w = std::max(60, topClient.GetWidth() - x);

    int y = rowClient.y;
    if (y + h > topClient.GetHeight())
        y = std::max(0, topClient.GetHeight() - h);

    m_subMenu->SetSize(wxSize(w, h));
    m_subMenu->SetPosition(wxPoint(x, y));
}

void StartMenu::CreateMenuContent() {
    m_scrollArea->DestroyChildren();
    m_menuRows.clear();
    HideSubMenu();

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    std::set<std::string> usedUris;

    auto iconRecent = os::app.getIconTheme()->icon("shell", "startmenu.recent").toBitmap1(kIconSize, kIconSize);
    auto iconTrash = os::app.getIconTheme()->icon("shell", "startmenu.trash").toBitmap1(kIconSize, kIconSize);
    auto iconExit = os::app.getIconTheme()->icon("shell", "startmenu.exit").toBitmap1(kIconSize, kIconSize);

    auto addModule = [&](ModulePtr m) {
        if (!m || usedUris.count(m->getFullUri()))
            return;
        usedUris.insert(m->getFullUri());
        auto b = m->image.toBitmap1(kIconSize, kIconSize);
        AddMenuItem(m_scrollArea, sizer, m->label, m, false, b.IsOk() ? &b : nullptr, ROW_LEAF,
                    ID_CATEGORY_NONE);
    };
    auto addPseudo = [&](const wxString& text, const wxBitmap* icon, RowKind kind) {
        AddMenuItem(m_scrollArea, sizer, text, nullptr, false, icon, kind, ID_CATEGORY_NONE);
    };
    auto addSep = [&]() {
        AddMenuItem(m_scrollArea, sizer, "", nullptr, true, nullptr, ROW_LEAF, ID_CATEGORY_NONE);
    };
    auto findByName = [&](const std::string& name) -> ModulePtr {
        for (const auto& m : m_filteredModules)
            if (m && m->name == name)
                return m;
        return nullptr;
    };

    if (m_filteredModules.empty()) {
        m_scrollArea->SetSizer(sizer);
        m_scrollArea->Layout();
        return;
    }

    ModulePtr explorer = findByName("explorer");
    if (explorer) {
        addModule(explorer);
        addSep();
    }
    for (const auto& cat : getAllCategories()) {
        bool any = false;
        for (const auto& m : m_filteredModules)
            if (m && m->categoryId == cat.id && !usedUris.count(m->getFullUri())) {
                any = true;
                break;
            }
        if (!any)
            continue;
        auto catIcon = cat.icon.toBitmap1(kIconSize, kIconSize);
        AddMenuItem(m_scrollArea, sizer, cat.label, nullptr, false,
                    catIcon.IsOk() ? &catIcon : nullptr, ROW_CATEGORY_FOLDER, cat.id);
    }
    addSep();
    addPseudo("Recent files", iconRecent.IsOk() ? &iconRecent : nullptr, ROW_RECENT_FOLDER);
    addPseudo("Trash", iconTrash.IsOk() ? &iconTrash : nullptr, ROW_LEAF);
    addSep();
    addModule(findByName("console"));
    addModule(findByName("controlpanel"));
    addModule(findByName("registry"));
    addSep();
    addPseudo("Exit", iconExit.IsOk() ? &iconExit : nullptr, ROW_LEAF);

    m_scrollArea->SetSizer(sizer);
    m_scrollArea->Layout();
}

} // namespace os

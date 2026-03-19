#ifndef OMNISHELL_SHELL_START_MENU_HPP
#define OMNISHELL_SHELL_START_MENU_HPP

#include "../core/Category.hpp"
#include "../core/Module.hpp"

#include <wx/textctrl.h>
#include <wx/wx.h>

#include <functional>
#include <vector>

namespace os {

/**
 * Start Menu (XP-style)
 *
 * Implemented as a panel that is typically shown as an overlay (not sizer-managed):
 * - Positioned above the taskbar, left-aligned; hidden by default
 * - Hides when mouse leaves or user clicks outside
 * - Menu-style rows: icon + label, submenu ">", expandable sections
 */
class StartMenu : public wxPanel {
public:
    StartMenu(wxWindow* parent);
    virtual ~StartMenu();

    /** Show menu (overlay; does not rely on sizer visibility) */
    void ShowMenu();
    /** Hide menu (overlay; does not rely on sizer visibility) */
    void HideMenu();

    /** True if a screen point is inside menu or its submenu overlay. */
    bool ContainsScreenPoint(const wxPoint& screenPt) const;

    void populateModules(const std::vector<ModulePtr>& modules);
    void clearModules();

    using LaunchCallback = std::function<void(ModulePtr)>;
    void setLaunchCallback(LaunchCallback callback);

    /** Handle key presses coming from the parent frame when the menu is open. */
    bool HandleGlobalKey(wxKeyEvent& event);

protected:
    void OnSearch(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnLeaveWindow(wxMouseEvent& event);
    void OnSubMenuLeaveWindow(wxMouseEvent& event);

    enum RowKind { ROW_LEAF, ROW_CATEGORY_FOLDER, ROW_RECENT_FOLDER };
    struct MenuRow { wxWindow* row; ModulePtr module; RowKind kind; CategoryId categoryId; };

    void FilterModules(const std::string& searchText);
    void CreateMenuContent();
    void AddMenuItem(wxWindow* parent, wxSizer* sizer, const wxString& label,
                     ModulePtr module, bool isSeparator,
                     const wxBitmap* iconBitmap, RowKind rowKind, CategoryId categoryId);
    void OnMenuItemClick(wxMouseEvent& event);
    void OnMenuItemEnter(wxMouseEvent& event);
    void OnMenuItemLeave(wxMouseEvent& event);
    void ShowSubMenuForRow(wxWindow* row);
    void HideSubMenu();
    void BuildSubMenuContent(RowKind kind, CategoryId categoryId);
    void PositionSubMenuNearRow(wxWindow* row);

    wxTextCtrl* searchBox_;
    wxPanel* scrollArea_;
    wxPanel* categoryPanel_;

    std::vector<ModulePtr> allModules_;
    std::vector<ModulePtr> filteredModules_;
    std::vector<MenuRow> menuRows_;
    LaunchCallback launchCallback_;

    CategoryId activeCategoryId_;
    // Submenu (right-side) overlay for "folder" rows.
    wxPanel* subMenu_;
    wxPanel* subScrollArea_;
    wxSizer* subSizer_;
    RowKind subMenuKind_;
    CategoryId subMenuCategoryId_;
    std::vector<ModulePtr> subMenuModules_;

    int menuWidth_;
    int menuHeight_;
};

} // namespace os

#endif // OMNISHELL_SHELL_START_MENU_HPP

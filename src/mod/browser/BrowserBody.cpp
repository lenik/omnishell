#include "BrowserBody.hpp"

#include "../../shell/Shell.hpp"
#include "../../shell/VfsWebViewHandlers.hpp"

#include <bas/wx/uiframe.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/accel.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/log.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/uri.h>
#include <wx/webview.h>
#include <wx/event.h>

#include <cctype>
#include <cstdio>
#include <string>
#include <string_view>

namespace os {

namespace {

std::string wxToUtf8(const wxString& w) {
    const wxScopedCharBuffer buf = w.utf8_str();
    return std::string(buf.data(), buf.length());
}

std::string encodeUriBytes(std::string_view path) {
    std::string out;
    out.reserve(path.size() + 8);
    for (unsigned char c : path) {
        if (std::isalnum(c) || c == '/' || c == '-' || c == '.' || c == '_')
            out += static_cast<char>(c);
        else {
            char hex[5];
            std::snprintf(hex, sizeof(hex), "%%%02X", c);
            out += hex;
        }
    }
    return out;
}

enum {
    ID_URL_COMBO = wxID_HIGHEST + 900,
    ID_BTN_BACK,
    ID_BTN_FWD,
    ID_BTN_GO,
    ID_BTN_REFRESH,
    ID_BTN_HOME,
    ID_WEBVIEW,
    ID_WV_ACCEL_BACK,
    ID_WV_ACCEL_FWD,
};

} // namespace

BrowserBody::BrowserBody(VolumeManager* vm)
    : m_vm(vm) {}

void BrowserBody::updateNavButtons() {
    if (!m_web || !m_btnBack || !m_btnFwd)
        return;
    m_btnBack->Enable(m_web->CanGoBack());
    m_btnFwd->Enable(m_web->CanGoForward());
}

void BrowserBody::goBack() {
    if (m_web && m_web->CanGoBack()) {
        m_web->GoBack();
        updateNavButtons();
    }
}

void BrowserBody::goForward() {
    if (m_web && m_web->CanGoForward()) {
        m_web->GoForward();
        updateNavButtons();
    }
}

bool BrowserBody::handleBrowserNavKey(wxKeyEvent& e) {
    if (!e.AltDown() || e.ControlDown())
        return false;
    const int k = e.GetKeyCode();
    if (k == WXK_LEFT || k == WXK_NUMPAD_LEFT) {
        goBack();
        return true;
    }
    if (k == WXK_RIGHT || k == WXK_NUMPAD_RIGHT) {
        goForward();
        return true;
    }
    return false;
}

void BrowserBody::refreshPage() {
    if (m_web)
        m_web->Reload(wxWEBVIEW_RELOAD_NO_CACHE);
}

void BrowserBody::focusAddressBar() {
    if (!m_urlCombo)
        return;
    m_urlCombo->SetFocus();
    const wxString v = m_urlCombo->GetValue();
    if (v.empty())
        m_urlCombo->SetInsertionPointEnd();
    else
        m_urlCombo->SetSelection(0, static_cast<long>(v.length()));
}

void BrowserBody::createFragmentView(CreateViewContext* ctx) {
    wxWindow* parent = ctx->getParent();
    m_frame = dynamic_cast<uiFrame*>(parent);
    wxPanel* root = new wxPanel(parent, wxID_ANY, ctx->getPos(), ctx->getSize());
    wxBoxSizer* v = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* bar = new wxBoxSizer(wxHORIZONTAL);

    m_btnBack = new wxButton(root, ID_BTN_BACK, "<");
    m_btnBack->SetToolTip("Back");
    m_btnFwd = new wxButton(root, ID_BTN_FWD, ">");
    m_btnFwd->SetToolTip("Forward");

    m_urlCombo = new wxComboBox(root, ID_URL_COMBO, "", wxDefaultPosition, wxDefaultSize, 0, nullptr,
                                wxCB_DROPDOWN | wxTE_PROCESS_ENTER);
    m_urlCombo->SetHint("Address");

    auto* go = new wxButton(root, ID_BTN_GO, "Go");
    auto* refresh = new wxButton(root, ID_BTN_REFRESH, "Refresh");
    auto* home = new wxButton(root, ID_BTN_HOME, "Home");

    bar->Add(m_btnBack, 0, wxALL, 2);
    bar->Add(m_btnFwd, 0, wxALL, 2);
    bar->Add(m_urlCombo, 1, wxALL | wxEXPAND, 2);
    bar->Add(go, 0, wxALL, 2);
    bar->Add(refresh, 0, wxALL, 2);
    bar->Add(home, 0, wxALL, 2);
    v->Add(bar, 0, wxEXPAND);

    m_web = wxWebView::New(root, ID_WEBVIEW, wxWebViewDefaultURLStr, wxDefaultPosition, wxDefaultSize,
                           wxWebViewBackendDefault);
    m_web->EnableContextMenu(true);
    v->Add(m_web, 1, wxEXPAND);

    if (ShellApp* sh = ShellApp::getInstance()) {
        m_httpBase = sh->vfsDaemon().httpBase();
        if (m_web && m_vm && !m_httpBase.empty())
            RegisterDaemonWebViewHandlers(m_web, m_vm, m_httpBase);
    }

    // Alt+←/→: frame accelerators often miss the focused WebView; accels on the WebView + CHAR_HOOK cover GTK/MSW.
    wxAcceleratorEntry wvNav[2];
    wvNav[0].Set(wxACCEL_ALT, WXK_LEFT, ID_WV_ACCEL_BACK);
    wvNav[1].Set(wxACCEL_ALT, WXK_RIGHT, ID_WV_ACCEL_FWD);
    m_web->SetAcceleratorTable(wxAcceleratorTable(2, wvNav));
    m_web->Bind(wxEVT_MENU, [this](wxCommandEvent&) { goBack(); }, ID_WV_ACCEL_BACK);
    m_web->Bind(wxEVT_MENU, [this](wxCommandEvent&) { goForward(); }, ID_WV_ACCEL_FWD);

    root->SetSizer(v);

    m_btnBack->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { goBack(); });
    m_btnFwd->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { goForward(); });
    go->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onGo(); });
    refresh->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { refreshPage(); });
    home->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_httpBase.empty()) {
            wxLogWarning("Browser: VFS daemon not running");
            return;
        }
        navigateTo(wxString::FromUTF8(m_httpBase.data(), static_cast<int>(m_httpBase.size())) + "/volume/");
    });

    m_urlCombo->Bind(wxEVT_TEXT_ENTER, &BrowserBody::onUrlComboEnter, this);
    m_urlCombo->Bind(wxEVT_COMBOBOX, &BrowserBody::onHistoryPick, this);

    m_web->Bind(wxEVT_WEBVIEW_NAVIGATED, [this](wxWebViewEvent& ev) {
        m_updatingCombo = true;
        if (m_urlCombo)
            m_urlCombo->SetValue(ev.GetURL());
        m_updatingCombo = false;
        pushHistory(ev.GetURL());
        updateNavButtons();
    });
    m_web->Bind(wxEVT_WEBVIEW_TITLE_CHANGED, [this](wxWebViewEvent& ev) {
        if (m_frame)
            m_frame->SetTitle(ev.GetString().empty() ? "Browser" : ("Browser - " + ev.GetString()));
    });
    m_web->Bind(wxEVT_WEBVIEW_NEWWINDOW, [this](wxWebViewEvent& ev) { navigateTo(ev.GetURL()); });
    m_web->Bind(wxEVT_WEBVIEW_ERROR, [](wxWebViewEvent& ev) {
        wxLogWarning("WebView error: %s", ev.GetURL());
    });

    // Embedded WebView (especially GTK WebKit) often does not let frame accelerator tables see Alt+arrows.
    if (m_frame) {
        m_frame->Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
            if (handleBrowserNavKey(e))
                e.Skip(false);
            else
                e.Skip();
        });
    }

    updateNavButtons();
}

wxEvtHandler* BrowserBody::getEventHandler() {
    return m_web ? m_web->GetEventHandler() : (m_urlCombo ? m_urlCombo->GetEventHandler() : nullptr);
}

wxString BrowserBody::mapVirtualToHttp(const wxString& url) const {
    if (m_httpBase.empty())
        return url;

    wxString u = url;
    u.Trim(true).Trim(false);
    if (u.StartsWith("http://") || u.StartsWith("https://") || u.StartsWith("file://"))
        return u;

    if (u.StartsWith("vol://")) {
        wxURI uri(u);
        if (!uri.HasScheme() || uri.GetScheme().Lower() != "vol")
            return url;
        if (!uri.HasServer())
            return url;
        const std::string idx = wxToUtf8(uri.GetServer());
        wxString pathWx = uri.GetPath();
        while (pathWx.StartsWith("//"))
            pathWx = pathWx.Mid(1);
        std::string path = wxToUtf8(pathWx);
        if (!path.empty() && path[0] == '/')
            path = path.substr(1);

        std::string s = m_httpBase + "/volume/" + idx;
        if (!path.empty())
            s += "/" + encodeUriBytes(path);
        return wxString::FromUTF8(s.data(), static_cast<int>(s.size()));
    }

    if (u.StartsWith("asset://")) {
        wxURI uri(u);
        wxString pathWx = uri.GetPath();
        while (pathWx.StartsWith("//"))
            pathWx = pathWx.Mid(1);
        std::string path = wxToUtf8(pathWx);
        if (!path.empty() && path[0] == '/')
            path = path.substr(1);

        std::string s = m_httpBase + "/asset";
        if (!path.empty())
            s += "/" + encodeUriBytes(path);
        return wxString::FromUTF8(s.data(), static_cast<int>(s.size()));
    }

    return url;
}

void BrowserBody::navigateTo(const wxString& url) {
    if (!m_web || url.empty())
        return;
    wxString u = url;
    u.Trim(true).Trim(false);

    if (u.StartsWith("vol://") || u.StartsWith("asset://")) {
        if (m_httpBase.empty()) {
            wxLogWarning("Browser: cannot load %s (VFS daemon not running)", u);
            return;
        }
        m_web->LoadURL(u);
        return;
    }

    if (!u.StartsWith("http://") && !u.StartsWith("https://") && !u.StartsWith("file://")) {
        if (u.StartsWith("/")) {
            m_web->LoadURL(wxString("file://") + u);
            return;
        }
        m_web->LoadURL(wxString("https://") + u);
        return;
    }

    m_web->LoadURL(mapVirtualToHttp(u));
}

void BrowserBody::onGo() {
    if (!m_urlCombo)
        return;
    navigateTo(m_urlCombo->GetValue());
}

void BrowserBody::onHistoryPick(wxCommandEvent&) {
    if (m_updatingCombo)
        return;
    onGo();
}

void BrowserBody::onUrlComboEnter(wxCommandEvent&) {
    onGo();
}

void BrowserBody::pushHistory(const wxString& url) {
    if (url.empty())
        return;
    for (auto it = m_history.begin(); it != m_history.end();) {
        if (*it == url)
            it = m_history.erase(it);
        else
            ++it;
    }
    m_history.insert(m_history.begin(), url);
    while (m_history.size() > kMaxHistory)
        m_history.pop_back();

    if (!m_urlCombo)
        return;
    m_updatingCombo = true;
    m_urlCombo->Clear();
    for (const wxString& h : m_history)
        m_urlCombo->Append(h);
    m_urlCombo->SetValue(url);
    m_updatingCombo = false;
}

} // namespace os

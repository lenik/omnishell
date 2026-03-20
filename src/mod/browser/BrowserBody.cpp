#include "BrowserBody.hpp"

#include <bas/wx/uiframe.hpp>
#include <bas/volume/FileStatus.hpp>
#include <bas/volume/MemoryZip.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>
#include <bas/proc/Assets.hpp>

#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/log.h>
#include <wx/stattext.h>
#include <wx/filesys.h>
#include <wx/mstream.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/uri.h>
#include <wx/webview.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string>

namespace os {

namespace {

std::string joinPath(const std::string& base, const std::string& name) {
    if (base.empty() || base == "/")
        return "/" + name;
    if (base.back() == '/')
        return base + name;
    return base + "/" + name;
}

std::string parentPath(const std::string& p) {
    if (p.empty() || p == "/")
        return "/";
    std::string s = p;
    while (s.size() > 1 && s.back() == '/')
        s.pop_back();
    size_t slash = s.rfind('/');
    if (slash == std::string::npos || slash == 0)
        return "/";
    return s.substr(0, slash);
}

std::string wxToUtf8(const wxString& w) {
    const wxScopedCharBuffer buf = w.utf8_str();
    return std::string(buf.data(), buf.length());
}

wxString utf8Wx(const std::string& s) {
    return wxString::FromUTF8(s.data(), s.size());
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

wxString htmlEscapeWx(const wxString& s) {
    wxString o;
    for (wxUniChar uc : s) {
        const wxChar c = static_cast<wxChar>(uc);
        if (c == '&')
            o += "&amp;";
        else if (c == '<')
            o += "&lt;";
        else if (c == '>')
            o += "&gt;";
        else if (c == '"')
            o += "&quot;";
        else
            o += c;
    }
    return o;
}

wxString mimeForPath(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos)
        return "application/octet-stream";
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
    if (ext == "html" || ext == "htm")
        return "text/html";
    if (ext == "xhtml")
        return "application/xhtml+xml";
    if (ext == "css")
        return "text/css";
    if (ext == "js" || ext == "mjs")
        return "application/javascript";
    if (ext == "json")
        return "application/json";
    if (ext == "svg")
        return "image/svg+xml";
    if (ext == "png")
        return "image/png";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "gif")
        return "image/gif";
    if (ext == "webp")
        return "image/webp";
    if (ext == "ico")
        return "image/x-icon";
    if (ext == "txt" || ext == "md")
        return "text/plain";
    if (ext == "xml")
        return "application/xml";
    if (ext == "wasm")
        return "application/wasm";
    return "application/octet-stream";
}

wxFSFile* makeHtmlResponse(const wxString& uri, const wxString& title, const wxString& innerHtml) {
    wxString html = wxString::Format(
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/><title>%s</title></head><body>%s</body></html>",
        htmlEscapeWx(title), innerHtml);
    const wxScopedCharBuffer buf = html.utf8_str();
    return new wxFSFile(new wxMemoryInputStream(buf.data(), buf.length()), uri, "text/html", wxEmptyString,
                        wxDateTime::Now());
}

class VolSchemeHandler : public wxWebViewHandler {
  public:
    explicit VolSchemeHandler(VolumeManager* vm)
        : wxWebViewHandler("vol")
        , m_vm(vm) {}

    wxFSFile* GetFile(const wxString& uriWx) override {
        if (!m_vm)
            return makeHtmlResponse(uriWx, "Error", "<p>No volume manager.</p>");

        wxURI u(uriWx);
        if (!u.HasScheme() || u.GetScheme().Lower() != "vol")
            return nullptr;
        if (!u.HasServer())
            return makeHtmlResponse(uriWx, "Bad URL", "<p>Expected vol://&lt;index&gt;/path</p>");

        long idxL = 0;
        if (!u.GetServer().ToLong(&idxL) || idxL < 0)
            return makeHtmlResponse(uriWx, "Bad volume index", "<p>Volume index must be a non-negative integer.</p>");

        const size_t idx = static_cast<size_t>(idxL);
        if (idx >= m_vm->getVolumeCount())
            return makeHtmlResponse(uriWx, "Volume not found", "<p>Volume index out of range.</p>");

        Volume* vol = m_vm->getVolume(idx);
        wxString pathWx = u.GetPath();
        if (pathWx.empty())
            pathWx = "/";
        while (pathWx.StartsWith("//"))
            pathWx = pathWx.Mid(1);

        std::string path;
        try {
            path = vol->normalize(wxToUtf8(pathWx), true);
        } catch (...) {
            return makeHtmlResponse(uriWx, "Bad path", "<p>Could not normalize path.</p>");
        }

        try {
            if (vol->isDirectory(path)) {
                auto entries = vol->readDir(path, false);
                std::sort(entries.begin(), entries.end(),
                          [](const std::unique_ptr<FileStatus>& a, const std::unique_ptr<FileStatus>& b) {
                              if (a->isDirectory() != b->isDirectory())
                                  return a->isDirectory() > b->isDirectory();
                              return a->name < b->name;
                          });

                wxString list;
                list << "<ul style=\"font-family:system-ui,sans-serif\">";
                const wxString parent =
                    wxString::Format("vol://%ld%s", idxL, utf8Wx(encodeUriBytes(parentPath(path))));
                list << "<li><a href=\"" << htmlEscapeWx(parent) << "\">Parent directory</a></li>";

                for (const auto& e : entries) {
                    const std::string child = joinPath(path, e->name);
                    const std::string enc = encodeUriBytes(child);
                    wxString href = wxString::Format("vol://%ld%s", idxL, utf8Wx(enc));
                    wxString label = htmlEscapeWx(utf8Wx(e->name));
                    if (e->isDirectory())
                        label << "/";
                    list << "<li><a href=\"" << htmlEscapeWx(href) << "\">" << label << "</a></li>";
                }
                list << "</ul>";

                wxString title = wxString::Format("Index of vol://%ld%s", idxL, utf8Wx(path));
                wxString body;
                body << "<h1>" << htmlEscapeWx(title) << "</h1>" << list;
                return makeHtmlResponse(uriWx, title, body);
            }

            if (!vol->isFile(path))
                return makeHtmlResponse(uriWx, "Not found", "<p>Path does not exist or is not a file.</p>");

            std::vector<uint8_t> data = vol->readFile(path);
            wxString mime = mimeForPath(path);
            return new wxFSFile(new wxMemoryInputStream(data.data(), data.size()), uriWx, mime, wxEmptyString,
                                wxDateTime::Now());
        } catch (const std::exception& ex) {
            return makeHtmlResponse(uriWx, "Error", wxString::Format("<p>%s</p>", htmlEscapeWx(utf8Wx(ex.what()))));
        } catch (...) {
            return makeHtmlResponse(uriWx, "Error", "<p>Unknown error reading volume.</p>");
        }
    }

  private:
    VolumeManager* m_vm;
};

class AssetSchemeHandler : public wxWebViewHandler {
  public:
    AssetSchemeHandler()
        : wxWebViewHandler("asset") {}

    wxFSFile* GetFile(const wxString& uriWx) override {
        MemoryZip* z = assets.get();
        if (!z)
            return makeHtmlResponse(uriWx, "Assets", "<p>Embedded assets volume is not available.</p>");

        wxURI u(uriWx);
        if (!u.HasScheme() || u.GetScheme().Lower() != "asset")
            return nullptr;

        wxString pathWx = u.GetPath();
        while (pathWx.StartsWith("//"))
            pathWx = pathWx.Mid(1);
        if (pathWx.empty())
            pathWx = "/";

        std::string path;
        try {
            path = z->normalize(wxToUtf8(pathWx), true);
        } catch (...) {
            return makeHtmlResponse(uriWx, "Bad path", "<p>Could not normalize path.</p>");
        }

        try {
            if (z->isDirectory(path)) {
                auto entries = z->readDir(path, false);
                std::sort(entries.begin(), entries.end(),
                          [](const std::unique_ptr<FileStatus>& a, const std::unique_ptr<FileStatus>& b) {
                              if (a->isDirectory() != b->isDirectory())
                                  return a->isDirectory() > b->isDirectory();
                              return a->name < b->name;
                          });

                wxString list;
                list << "<ul style=\"font-family:system-ui,sans-serif\">";
                const wxString parent = utf8Wx(encodeUriBytes(parentPath(path)));
                list << "<li><a href=\"" << htmlEscapeWx(wxString("asset://") + parent) << "\">Parent directory</a></li>";

                for (const auto& e : entries) {
                    const std::string child = joinPath(path, e->name);
                    const std::string enc = encodeUriBytes(child);
                    wxString href = wxString("asset://") + utf8Wx(enc);
                    wxString label = htmlEscapeWx(utf8Wx(e->name));
                    if (e->isDirectory())
                        label << "/";
                    list << "<li><a href=\"" << htmlEscapeWx(href) << "\">" << label << "</a></li>";
                }
                list << "</ul>";

                wxString title = wxString::Format("Index of asset://%s", utf8Wx(path));
                wxString body;
                body << "<h1>" << htmlEscapeWx(title) << "</h1>" << list;
                return makeHtmlResponse(uriWx, title, body);
            }

            if (!z->isFile(path))
                return makeHtmlResponse(uriWx, "Not found", "<p>Path does not exist or is not a file.</p>");

            std::vector<uint8_t> data = z->readFile(path);
            wxString mime = mimeForPath(path);
            return new wxFSFile(new wxMemoryInputStream(data.data(), data.size()), uriWx, mime, wxEmptyString,
                                wxDateTime::Now());
        } catch (const std::exception& ex) {
            return makeHtmlResponse(uriWx, "Error", wxString::Format("<p>%s</p>", htmlEscapeWx(utf8Wx(ex.what()))));
        } catch (...) {
            return makeHtmlResponse(uriWx, "Error", "<p>Unknown error reading assets.</p>");
        }
    }
};

enum {
    ID_URL_COMBO = wxID_HIGHEST + 900,
    ID_BTN_BACK,
    ID_BTN_FWD,
    ID_BTN_GO,
    ID_BTN_REFRESH,
    ID_BTN_HOME,
    ID_WEBVIEW,
};

} // namespace

BrowserBody::BrowserBody(VolumeManager* vm)
    : m_vm(vm) {}

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

    if (m_vm) {
        m_web->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new VolSchemeHandler(m_vm)));
    }
    m_web->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new AssetSchemeHandler()));

    root->SetSizer(v);

    auto updateNavButtons = [this] {
        if (!m_web || !m_btnBack || !m_btnFwd)
            return;
        m_btnBack->Enable(m_web->CanGoBack());
        m_btnFwd->Enable(m_web->CanGoForward());
    };

    m_btnBack->Bind(wxEVT_BUTTON, [this, updateNavButtons](wxCommandEvent&) {
        if (m_web && m_web->CanGoBack()) {
            m_web->GoBack();
            updateNavButtons();
        }
    });
    m_btnFwd->Bind(wxEVT_BUTTON, [this, updateNavButtons](wxCommandEvent&) {
        if (m_web && m_web->CanGoForward()) {
            m_web->GoForward();
            updateNavButtons();
        }
    });
    go->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onGo(); });
    refresh->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_web)
            m_web->Reload(wxWEBVIEW_RELOAD_DEFAULT);
    });
    home->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { navigateTo(wxString("asset:///")); });

    m_urlCombo->Bind(wxEVT_TEXT_ENTER, &BrowserBody::onUrlComboEnter, this);
    m_urlCombo->Bind(wxEVT_COMBOBOX, &BrowserBody::onHistoryPick, this);

    m_web->Bind(wxEVT_WEBVIEW_NAVIGATED, [this, updateNavButtons](wxWebViewEvent& ev) {
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
    m_web->Bind(wxEVT_WEBVIEW_NEWWINDOW, [this](wxWebViewEvent& ev) {
        navigateTo(ev.GetURL());
    });
    m_web->Bind(wxEVT_WEBVIEW_ERROR, [](wxWebViewEvent& ev) {
        wxLogWarning("WebView error: %s", ev.GetURL());
    });

    updateNavButtons();
}

wxEvtHandler* BrowserBody::getEventHandler() {
    return m_web ? m_web->GetEventHandler() : (m_urlCombo ? m_urlCombo->GetEventHandler() : nullptr);
}

void BrowserBody::navigateTo(const wxString& url) {
    if (!m_web || url.empty())
        return;
    wxString u = url;
    u.Trim(true).Trim(false);
    if (u.StartsWith("vol://") || u.StartsWith("asset://") || u.StartsWith("http://") || u.StartsWith("https://") ||
        u.StartsWith("file://")) {
        m_web->LoadURL(u);
        return;
    }
    if (u.StartsWith("/")) {
        m_web->LoadURL(wxString("file://") + u);
        return;
    }
    m_web->LoadURL(wxString("https://") + u);
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

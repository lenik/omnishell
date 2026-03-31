#include "VfsWebViewHandlers.hpp"

#include "../daemon/AssetController.hpp"
#include "../daemon/DirIndex.hpp"
#include "../daemon/FileController.hpp"
#include "../daemon/VolumeIndex.hpp"

#include <bas/proc/AssetsRegistry.hpp>
#include <bas/volume/Volume.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <wx/filesys.h>
#include <wx/mstream.h>
#include <wx/uri.h>
#include <wx/webview.h>

#include <memory>
#include <string>
#include <vector>

namespace os {

namespace {

void replaceAll(std::string& s, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.length(), to);
        pos += to.length();
    }
}

void rewriteVolumeListingForVolScheme(std::string& html, VolumeManager* vm, const std::string& httpBase) {
    if (!vm)
        return;
    for (size_t i = 0; i < vm->getVolumeCount(); ++i) {
        const std::string from = "\"/volume/" + std::to_string(i) + "/";
        const std::string to = "\"vol://" + std::to_string(i) + "/";
        replaceAll(html, from, to);
    }
    replaceAll(html, "href=\"/volume/\"", std::string("href=\"") + httpBase + "/volume/\"");
}

void rewriteAssetListingForAssetScheme(std::string& html) {
    replaceAll(html, "href=\"/asset/\"", "href=\"asset:///\"");
    replaceAll(html, "href=\"/asset/", "href=\"asset:///");
    replaceAll(html, "src=\"/asset/", "src=\"asset:///");
}

std::string wxToUtf8(const wxString& w) {
    const wxScopedCharBuffer buf = w.utf8_str();
    return std::string(buf.data(), buf.length());
}

wxString utf8Wx(const std::string& s) {
    return wxString::FromUTF8(s.data(), static_cast<int>(s.size()));
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

/** Build HTML as UTF-8 via wxMemoryOutputStream → wxMemoryInputStream (copies bytes; safe lifetime). */
wxFSFile* makeHtmlFsFile(const wxString& uriWx, const wxString& title, const wxString& innerHtml) {
    wxString html = wxString::Format(
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/><title>%s</title></head><body>%s</body></html>",
        htmlEscapeWx(title), innerHtml);
    const wxScopedCharBuffer u8 = html.utf8_str();
    wxMemoryOutputStream mos;
    mos.Write(u8.data(), u8.length());
    return new wxFSFile(new wxMemoryInputStream(mos), uriWx, "text/html", wxEmptyString, wxDateTime::Now());
}

wxFSFile* makeHtmlFsFileUtf8(const wxString& uriWx, const std::string& utf8Html) {
    wxMemoryOutputStream mos;
    mos.Write(utf8Html.data(), utf8Html.size());
    return new wxFSFile(new wxMemoryInputStream(mos), uriWx, "text/html", wxEmptyString, wxDateTime::Now());
}

/** Copy file bytes (SVG, images, etc.); wxMemoryInputStream(ptr,len) would dangle after GetFile returns. */
wxFSFile* makeBinaryFsFile(const wxString& uriWx, const std::vector<uint8_t>& data, const wxString& mimeWx) {
    wxMemoryOutputStream mos;
    if (!data.empty())
        mos.Write(data.data(), data.size());
    return new wxFSFile(new wxMemoryInputStream(mos), uriWx, mimeWx, wxEmptyString, wxDateTime::Now());
}

class VolSchemeHandler : public wxWebViewHandler {
  public:
    VolSchemeHandler(VolumeManager* vm, std::string httpBase)
        : wxWebViewHandler("vol")
        , m_vm(vm)
        , m_httpBase(std::move(httpBase))
        , m_volumeIndex(std::make_unique<VolumeIndex>(vm))
        , m_dirIndex(std::make_unique<DirIndex>(vm))
        , m_files(std::make_unique<FileController>(vm)) {}

    wxFSFile* GetFile(const wxString& uriWx) override {
        if (!m_vm)
            return makeHtmlFsFile(uriWx, "Error", "<p>No volume manager.</p>");

        wxURI u(uriWx);
        if (!u.HasScheme() || u.GetScheme().Lower() != "vol")
            return nullptr;

        // vol:///… or vol:// (no authority): show volume list with indices (like HTTP /volume/).
        const wxString server = u.HasServer() ? u.GetServer() : wxString();
        if (server.empty()) {
            std::string html = m_volumeIndex->handleGet();
            rewriteVolumeListingForVolScheme(html, m_vm, m_httpBase);
            return makeHtmlFsFileUtf8(uriWx, html);
        }

        long idxL = 0;
        if (!server.ToLong(&idxL) || idxL < 0)
            return makeHtmlFsFile(uriWx, "Bad volume index", "<p>Volume index must be a non-negative integer.</p>");

        const size_t idx = static_cast<size_t>(idxL);
        if (idx >= m_vm->getVolumeCount())
            return makeHtmlFsFile(uriWx, "Volume not found", "<p>Volume index out of range.</p>");

        wxString pathWx = u.GetPath();
        if (pathWx.empty())
            pathWx = "/";
        while (pathWx.StartsWith("//"))
            pathWx = pathWx.Mid(1);

        std::string pathInfo;
        try {
            std::string norm = m_vm->getVolume(idx)->normalizeArg(wxToUtf8(pathWx), "/");
            if (norm.empty() || norm == "/")
                pathInfo.clear();
            else if (norm.front() == '/')
                pathInfo = norm.substr(1);
            else
                pathInfo = norm;
        } catch (...) {
            return makeHtmlFsFile(uriWx, "Bad path", "<p>Could not normalize path.</p>");
        }

        try {
            auto* vol = m_vm->getVolume(idx);
            const std::string volPath = pathInfo.empty() ? "/" : "/" + pathInfo;
            if (vol->isDirectory(volPath)) {
                std::string html = m_dirIndex->handleGet(idx, pathInfo);
                rewriteVolumeListingForVolScheme(html, m_vm, m_httpBase);
                return makeHtmlFsFileUtf8(uriWx, html);
            }
            if (!vol->isFile(volPath))
                return makeHtmlFsFile(uriWx, "Not found", "<p>Path does not exist or is not a file.</p>");

            std::vector<uint8_t> data;
            std::string mime;
            if (!m_files->readFilePayload(idx, pathInfo, data, mime))
                return makeHtmlFsFile(uriWx, "Error", "<p>Could not read file.</p>");

            return makeBinaryFsFile(uriWx, data, utf8Wx(mime));
        } catch (const std::exception& ex) {
            return makeHtmlFsFile(uriWx, "Error",
                                  utf8Wx("<p>") + htmlEscapeWx(utf8Wx(std::string(ex.what()))) + "</p>");
        } catch (...) {
            return makeHtmlFsFile(uriWx, "Error", "<p>Unknown error reading volume.</p>");
        }
    }

  private:
    VolumeManager* m_vm;
    std::string m_httpBase;
    std::unique_ptr<VolumeIndex> m_volumeIndex;
    std::unique_ptr<DirIndex> m_dirIndex;
    std::unique_ptr<FileController> m_files;
};

class AssetSchemeHandler : public wxWebViewHandler {
  public:
    AssetSchemeHandler()
        : wxWebViewHandler("asset")
        , m_assets(AssetsRegistry::instance().get()) {
        if (m_assets)
            m_controller = std::make_unique<AssetController>(m_assets);
    }

    wxFSFile* GetFile(const wxString& uriWx) override {
        if (!m_controller || !m_assets)
            return makeHtmlFsFile(uriWx, "Assets", "<p>Embedded assets volume is not available.</p>");

        wxURI u(uriWx);
        if (!u.HasScheme() || u.GetScheme().Lower() != "asset")
            return nullptr;

        wxString pathWx = u.GetPath();
        while (pathWx.StartsWith("//"))
            pathWx = pathWx.Mid(1);
        if (pathWx.empty())
            pathWx = "/";

        std::string pathInfo;
        try {
            std::string norm = m_assets->normalizeArg(wxToUtf8(pathWx), "/");
            if (norm.empty() || norm == "/")
                pathInfo.clear();
            else if (norm.front() == '/')
                pathInfo = norm.substr(1);
            else
                pathInfo = norm;
        } catch (...) {
            return makeHtmlFsFile(uriWx, "Bad path", "<p>Could not normalize path.</p>");
        }

        try {
            std::string ap = pathInfo.empty() ? "/" : "/" + pathInfo;
            if (m_assets->isDirectory(ap)) {
                std::string html = m_controller->generateDirectoryListing(ap, pathInfo);
                rewriteAssetListingForAssetScheme(html);
                return makeHtmlFsFileUtf8(uriWx, html);
            }
            if (!m_assets->isFile(ap))
                return makeHtmlFsFile(uriWx, "Not found", "<p>Path does not exist or is not a file.</p>");

            std::vector<uint8_t> data;
            std::string mime;
            if (!m_controller->readFilePayload(pathInfo, data, mime))
                return makeHtmlFsFile(uriWx, "Error", "<p>Could not read file.</p>");

            return makeBinaryFsFile(uriWx, data, utf8Wx(mime));
        } catch (const std::exception& ex) {
            return makeHtmlFsFile(uriWx, "Error",
                                  utf8Wx("<p>") + htmlEscapeWx(utf8Wx(std::string(ex.what()))) + "</p>");
        } catch (...) {
            return makeHtmlFsFile(uriWx, "Error", "<p>Unknown error reading assets.</p>");
        }
    }

  private:
    Volume* m_assets;
    std::unique_ptr<AssetController> m_controller;
};

} // namespace

void RegisterDaemonWebViewHandlers(wxWebView* web, VolumeManager* vm, const std::string& httpBase) {
    if (!web)
        return;

    // Always register `asset://` (embedded resources). It doesn't require VFS daemon / httpBase.
    web->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new AssetSchemeHandler()));

    // Register `vol://` only when VFS daemon is available.
    if (vm && !httpBase.empty())
        web->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new VolSchemeHandler(vm, httpBase)));
}

} // namespace os

#include "LocaleSetup.hpp"

#include "config.h"

#include <cstdlib>
#include <cstring>

namespace os {

namespace {

wxLanguage languageFromEnv() {
    const char* env = std::getenv("OMNISHELL_LANG");
    if (!env || !env[0])
        return wxLANGUAGE_DEFAULT;
    if (!std::strcmp(env, "en") || !std::strcmp(env, "en_US") || !std::strcmp(env, "en_GB"))
        return wxLANGUAGE_ENGLISH;
    if (!std::strcmp(env, "zh_CN") || !std::strcmp(env, "zh-Hans") || !std::strcmp(env, "zh_CN.UTF-8"))
        return wxLANGUAGE_CHINESE_CHINA;
    if (!std::strcmp(env, "zh_TW") || !std::strcmp(env, "zh-Hant") || !std::strcmp(env, "zh_HK") ||
        !std::strcmp(env, "zh_TW.UTF-8"))
        return wxLANGUAGE_CHINESE_TAIWAN;
    if (!std::strcmp(env, "ko") || !std::strcmp(env, "ko_KR") || !std::strcmp(env, "ko-KR"))
        return wxLANGUAGE_KOREAN;
    if (!std::strcmp(env, "ja") || !std::strcmp(env, "ja_JP") || !std::strcmp(env, "ja-JP"))
        return wxLANGUAGE_JAPANESE;
    return wxLANGUAGE_DEFAULT;
}

wxLanguage normalizeSupported(wxLanguage lang) {
    switch (lang) {
    case wxLANGUAGE_CHINESE:
    case wxLANGUAGE_CHINESE_SIMPLIFIED:
    case wxLANGUAGE_CHINESE_CHINA:
        return wxLANGUAGE_CHINESE_CHINA;
    case wxLANGUAGE_CHINESE_TAIWAN:
    case wxLANGUAGE_CHINESE_HONGKONG:
    case wxLANGUAGE_CHINESE_TRADITIONAL:
        return wxLANGUAGE_CHINESE_TAIWAN;
    case wxLANGUAGE_KOREAN:
        return wxLANGUAGE_KOREAN;
    case wxLANGUAGE_JAPANESE:
        return wxLANGUAGE_JAPANESE;
    default:
        return wxLANGUAGE_ENGLISH;
    }
}

wxLanguage effectiveLanguage() {
    wxLanguage fromEnv = languageFromEnv();
    if (fromEnv != wxLANGUAGE_DEFAULT)
        return normalizeSupported(fromEnv);

    int sys = wxLocale::GetSystemLanguage();
    if (sys == wxLANGUAGE_UNKNOWN || sys == wxLANGUAGE_DEFAULT)
        return wxLANGUAGE_ENGLISH;
    return normalizeSupported(static_cast<wxLanguage>(sys));
}

} // namespace

bool setupOmniShellLocale(wxLocale& locale) {
    if (const char* dir = std::getenv("OMNISHELL_LOCALEDIR")) {
        if (dir[0])
            wxLocale::AddCatalogLookupPathPrefix(wxString::FromUTF8(dir));
    }
    wxLocale::AddCatalogLookupPathPrefix(wxString::FromUTF8(OMNISHELL_BUILDTREE_LOCALEDIR));
    wxLocale::AddCatalogLookupPathPrefix(wxString::FromUTF8(OMNISHELL_LOCALEDIR));

    const wxLanguage lang = effectiveLanguage();
    const bool ok = locale.Init(lang, wxLOCALE_LOAD_DEFAULT);
    (void)locale.AddCatalog(wxT("omnishell"));
    return ok;
}

} // namespace os

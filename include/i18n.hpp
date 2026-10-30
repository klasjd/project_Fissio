#pragma once

#include <string>
#include <windows.h>

namespace fissio {

enum class Language {
    Korean = 0,
    English = 1,
    Chinese = 2,
    Japanese = 3,
    Spanish = 4,
    Russian = 5,
    Arabic = 6
};

struct UiStrings {
    // Window & Header
    std::wstring window_title;
    std::wstring header_title;
    std::wstring status_disconnected;
    std::wstring status_connected_wg;
    std::wstring status_connected_proxy;

    // Toggle Button
    std::wstring btn_start;
    std::wstring btn_stop;

    // Mode Selection Group
    std::wstring grp_mode;
    std::wstring rad_socks5;
    std::wstring rad_wg;
    std::wstring rad_hybrid;
    std::wstring btn_load_conf;
    std::wstring lbl_conf_default;
    std::wstring lbl_conf_selected_prefix;

    // Packet / Proxy Config Group
    std::wstring grp_config;
    std::wstring lbl_port;
    std::wstring lbl_split;
    std::wstring lbl_delay;
    std::wstring chk_sysproxy;
    std::wstring chk_startup;

    // Logs & Emergency Clean-up
    std::wstring grp_log;
    std::wstring btn_clear_log;
    std::wstring btn_cleanup;

    // Tray Menu
    std::wstring tray_status_connected;
    std::wstring tray_status_disconnected;
    std::wstring tray_toggle_disconnect;
    std::wstring tray_toggle_connect;
    std::wstring tray_cleanup;
    std::wstring tray_open;
    std::wstring tray_exit;
    std::wstring tray_lang_dialog;

    // Language Dialog ("실시간 언어 변경창")
    std::wstring btn_lang_dialog;
    std::wstring dlg_lang_title;
    std::wstring dlg_lang_desc;
    std::wstring dlg_lang_close;

    // Balloon Notifications
    std::wstring balloon_connected_title;
    std::wstring balloon_connected_info;
    std::wstring balloon_disconnected_title;
    std::wstring balloon_disconnected_info;
    std::wstring balloon_tray_minimize_title;
    std::wstring balloon_tray_minimize_info;
    std::wstring balloon_tray_close_title;
    std::wstring balloon_tray_close_info;

    // MessageBox Dialogs
    std::wstring dlg_conf_error_title;
    std::wstring dlg_conf_error_body;
    std::wstring dlg_cleanup_title;
    std::wstring dlg_cleanup_success_p1;
    std::wstring dlg_cleanup_success_p2;
    std::wstring dlg_cleanup_p2;
    std::wstring dlg_cleanup_p3;
    std::wstring dlg_cleanup_p4;
    std::wstring dlg_cleanup_footer;
    std::wstring dlg_wg_need_conf_title;
    std::wstring dlg_wg_need_conf_body;

    // Console logs
    std::wstring log_ready;
    std::wstring log_loaded_conf;
    std::wstring log_cleanup_start;
    std::wstring log_cleanup_killed;
    std::wstring log_cleanup_single;
    std::wstring log_cleanup_tray;
    std::wstring log_cleanup_proxy;
    std::wstring log_cleanup_done;
    std::wstring log_hybrid_active;
    std::wstring log_socks_active;
    std::wstring log_proxy_disabled;
    std::wstring log_service_stopped;
    std::wstring log_wg_started;
    std::wstring log_wg_failed;
    std::wstring log_wg_cancelled;
};

class I18nManager {
public:
    static I18nManager& instance() {
        static I18nManager inst;
        return inst;
    }

    Language current_language() const { return current_lang_; }

    void set_language(Language lang) {
        current_lang_ = lang;
        save_preference(lang);
    }

    const UiStrings& strings() const {
        return get_strings_for(current_lang_);
    }

    static const wchar_t* get_language_name(Language lang) {
        switch (lang) {
            case Language::Korean:   return L"🇰🇷 한국어";
            case Language::English:  return L"🇺🇸 English";
            case Language::Chinese:  return L"🇨🇳 简体中文";
            case Language::Japanese: return L"🇯🇵 日本語";
            case Language::Spanish:  return L"🇪🇸 Español";
            case Language::Russian:  return L"🇷🇺 Русский";
            case Language::Arabic:   return L"🇸🇦 العربية";
            default:                 return L"English";
        }
    }

private:
    Language current_lang_{Language::English};

    I18nManager() {
        current_lang_ = load_preference();
    }

    Language detect_system_language() {
        LANGID langId = GetUserDefaultUILanguage();
        WORD primary = PRIMARYLANGID(langId);
        switch (primary) {
            case LANG_KOREAN:   return Language::Korean;
            case LANG_CHINESE:  return Language::Chinese;
            case LANG_JAPANESE: return Language::Japanese;
            case LANG_SPANISH:  return Language::Spanish;
            case LANG_RUSSIAN:  return Language::Russian;
            case LANG_ARABIC:   return Language::Arabic;
            default:            return Language::English; // Fallback to English for unsupported languages
        }
    }

    void save_preference(Language lang) {
        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Fissio", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            DWORD val = static_cast<DWORD>(lang);
            RegSetValueExW(hKey, L"Language", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&val), sizeof(val));
            RegCloseKey(hKey);
        }
    }

    Language load_preference() {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Fissio", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
            DWORD val = 0;
            DWORD size = sizeof(val);
            DWORD type = REG_DWORD;
            if (RegQueryValueExW(hKey, L"Language", NULL, &type, reinterpret_cast<LPBYTE>(&val), &size) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                if (val <= 6) {
                    return static_cast<Language>(val);
                }
            } else {
                RegCloseKey(hKey);
            }
        }
        // Check user computer language; if unsupported, fallback to English
        return detect_system_language();
    }

    const UiStrings& get_strings_for(Language lang) const;
};

inline const UiStrings& tr() {
    return I18nManager::instance().strings();
}

} // namespace fissio

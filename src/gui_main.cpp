#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <sstream>
#include "proxy_controller.hpp"
#include "system_proxy.hpp"
#include "wireguard_tunnel.hpp"
#include "wireguard_config.hpp"
#include "advanced_config.hpp"
#include "i18n.hpp"
#include <dwmapi.h>
#include <uxtheme.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

// 100% Unified Neutral Dark Palette (Title bar, border, client area seamlessly match)
static const COLORREF COLOR_BG          = RGB(24, 24, 27);    // #18181B Neutral Zinc 900
static const COLORREF COLOR_CARD_BG     = RGB(36, 36, 42);    // #24242A Clean Card Surface
static const COLORREF COLOR_CARD_BORDER = RGB(55, 55, 65);    // #373741 Subtle Border
static const COLORREF COLOR_INPUT_BG    = RGB(16, 16, 18);    // #101012 Clean Input Surface
static const COLORREF COLOR_INPUT_BORDER= RGB(68, 68, 78);    // #44444E
static const COLORREF COLOR_TEXT_WHITE  = RGB(248, 250, 252); // #F8FAFC
static const COLORREF COLOR_TEXT_MUTED  = RGB(156, 163, 175); // #9CA3AF
static const COLORREF COLOR_TEXT_ACCENT = RGB(56, 189, 248);  // #38BDF8
static const COLORREF COLOR_EMERALD     = RGB(16, 185, 129);  // #10B981
static const COLORREF COLOR_EMERALD_LGT = RGB(52, 211, 153);  // #34D399
static const COLORREF COLOR_EMERALD_DRK = RGB(5, 150, 105);   // #059669
static const COLORREF COLOR_CRIMSON     = RGB(239, 68, 68);   // #EF4444
static const COLORREF COLOR_CRIMSON_LGT = RGB(248, 113, 113); // #F87171
static const COLORREF COLOR_CRIMSON_DRK = RGB(185, 28, 28);   // #B91C1C
static const COLORREF COLOR_INDIGO      = RGB(99, 102, 241);  // #6366F1
static const COLORREF COLOR_INDIGO_DRK  = RGB(79, 70, 229);   // #4F46E5

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_STATUS   2000
#define ID_TRAY_EXIT     2001
#define ID_TRAY_OPEN     2002
#define ID_TRAY_TOGGLE   2003
#define ID_TRAY_CLEANUP  2004
#define ID_TRAY_LANG_KO  2010
#define ID_TRAY_LANG_EN  2011
#define ID_TRAY_LANG_ZH  2012
#define ID_TRAY_LANG_JA  2013
#define ID_TRAY_LANG_ES  2014
#define ID_TRAY_LANG_RU  2016
#define ID_TRAY_LANG_AR  2017
#define ID_TRAY_LANG_DIALOG 2015

#define IDC_BTN_TOGGLE    1001
#define IDC_EDT_PORT      1002
#define IDC_EDT_SPLIT     1003
#define IDC_EDT_DELAY     1004
#define IDC_CHK_SYSPROXY  1005
#define IDC_EDT_LOG       1006
#define IDC_BTN_CLEAR     1007
#define IDC_RAD_SOCKS5    1008
#define IDC_RAD_WG        1009
#define IDC_RAD_HYBRID    1010
#define IDC_BTN_LOAD_CONF 1011
#define IDC_BTN_CLEANUP   1012
#define IDC_CMB_LANG      1013
#define IDC_BTN_LANG_DLG  1014
#define IDC_CHK_STARTUP   1015

static HINSTANCE g_hInst = NULL;
static HWND g_hWnd = NULL;
static HWND g_hTitle = NULL;
static HWND g_hCmbLang = NULL;
static HWND g_hBtnLangDlg = NULL;
static HWND g_hLangDlg = NULL;
static HWND g_hBtnToggle = NULL;
static HWND g_hBtnCleanup = NULL;
static HWND g_hLblStatus = NULL;
static HWND g_hGrpMode = NULL;
static HWND g_hEdtPort = NULL;
static HWND g_hEdtSplit = NULL;
static HWND g_hEdtDelay = NULL;
static HWND g_hChkSysProxy = NULL;
static HWND g_hChkStartup = NULL;
static HWND g_hEdtLog = NULL;
static HWND g_hRadSocks = NULL;
static HWND g_hRadWg = NULL;
static HWND g_hRadHybrid = NULL;
static HWND g_hBtnLoadConf = NULL;
static HWND g_hLblWgConf = NULL;
static HWND g_hGrpCfg = NULL;
static HWND g_hLblPort = NULL;
static HWND g_hLblSplit = NULL;
static HWND g_hLblDelay = NULL;
static HWND g_hGrpLog = NULL;
static HWND g_hBtnClear = NULL;

static NOTIFYICONDATAW g_nid{};
static bool g_trayNoticeShown = false;

static HFONT g_hFontNormal = NULL;
static HFONT g_hFontTitle = NULL;
static HFONT g_hFontStatus = NULL;
static HFONT g_hFontBtn = NULL;
static HFONT g_hFontSection = NULL;
static HFONT g_hFontConsole = NULL;
static HBRUSH g_hBrushBg = NULL;
static HBRUSH g_hBrushCard = NULL;
static HBRUSH g_hBrushInput = NULL;
static HPEN g_hPenCardBorder = NULL;
static HPEN g_hPenInputBorder = NULL;

static void apply_dark_window_attributes(HWND hwnd) {
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    DwmSetWindowAttribute(hwnd, 19, &darkMode, sizeof(darkMode));
    int corner = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    // Match title bar caption color and outer window border seamlessly to the window background
    COLORREF captionColor = COLOR_BG;
    COLORREF borderColor = COLOR_CARD_BORDER;
    COLORREF textColor = COLOR_TEXT_WHITE;
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &textColor, sizeof(textColor));
}

static LRESULT CALLBACK ModernBtnSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR) {
    switch (uMsg) {
    case WM_MOUSEMOVE: {
        if (!GetPropW(hwnd, L"HOVER")) {
            TRACKMOUSEEVENT tme{sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            SetPropW(hwnd, L"HOVER", (HANDLE)1);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    }
    case WM_MOUSELEAVE: {
        RemovePropW(hwnd, L"HOVER");
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }
    case WM_NCDESTROY: {
        RemovePropW(hwnd, L"HOVER");
        RemoveWindowSubclass(hwnd, ModernBtnSubclass, uIdSubclass);
        break;
    }
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static void draw_custom_button(DRAWITEMSTRUCT* dis);

static fissio::WireGuardConfig g_wgConfig{};
static bool g_hasWgConfig = false;
static std::string g_confPath;

// UTF-8 std::string -> UTF-16 std::wstring conversion helper
static std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

static void append_log_w(const std::wstring& line) {
    if (!g_hEdtLog) return;
    std::wstring text = line + L"\r\n";
    int len = GetWindowTextLengthW(g_hEdtLog);
    SendMessageW(g_hEdtLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(g_hEdtLog, EM_REPLACESEL, 0, (LPARAM)text.c_str());
}

// -------------------------------------------------------------
// Configuration Persistence & Auto-Loading
// -------------------------------------------------------------
static void save_last_conf_path(const std::wstring& wpath) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Fissio", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"LastConfPath", 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(wpath.c_str()),
                       static_cast<DWORD>((wpath.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }

    // Backup: Write to last_config.path in executable directory
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t* p = wcsrchr(exePath, L'\\');
    if (p) {
        *(p + 1) = L'\0';
        std::wstring backupPath = std::wstring(exePath) + L"last_config.path";
        FILE* fp = _wfopen(backupPath.c_str(), L"w, ccs=UTF-8");
        if (fp) {
            fputws(wpath.c_str(), fp);
            fclose(fp);
        }
    }
}

static std::wstring load_last_conf_path() {
    wchar_t buf[MAX_PATH] = {0};
    DWORD bufSize = sizeof(buf);
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Fissio", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        DWORD type = REG_SZ;
        if (RegQueryValueExW(hKey, L"LastConfPath", NULL, &type, reinterpret_cast<LPBYTE>(buf), &bufSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            if (wcslen(buf) > 0 && GetFileAttributesW(buf) != INVALID_FILE_ATTRIBUTES) {
                return std::wstring(buf);
            }
        } else {
            RegCloseKey(hKey);
        }
    }

    // Check backup file
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t* p = wcsrchr(exePath, L'\\');
    if (p) {
        *(p + 1) = L'\0';
        std::wstring backupPath = std::wstring(exePath) + L"last_config.path";
        FILE* fp = _wfopen(backupPath.c_str(), L"r, ccs=UTF-8");
        if (fp) {
            if (fgetws(buf, MAX_PATH, fp)) {
                size_t l = wcslen(buf);
                while (l > 0 && (buf[l - 1] == L'\r' || buf[l - 1] == L'\n')) {
                    buf[--l] = L'\0';
                }
                fclose(fp);
                if (wcslen(buf) > 0 && GetFileAttributesW(buf) != INVALID_FILE_ATTRIBUTES) {
                    return std::wstring(buf);
                }
            } else {
                fclose(fp);
            }
        }

        // Check default config.conf in working directory
        std::wstring defaultConf = std::wstring(exePath) + L"config.conf";
        if (GetFileAttributesW(defaultConf.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return defaultConf;
        }
        std::wstring defaultSample = std::wstring(exePath) + L"sample_wireguard.conf";
        if (GetFileAttributesW(defaultSample.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return defaultSample;
        }
    }

    return L"";
}

// -------------------------------------------------------------
// Windows Startup (Auto-Start) Registration Helpers
// -------------------------------------------------------------
static bool is_startup_enabled() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        wchar_t path[MAX_PATH];
        DWORD size = sizeof(path);
        DWORD type = REG_SZ;
        LONG res = RegQueryValueExW(hKey, L"Fissio", NULL, &type, (LPBYTE)path, &size);
        RegCloseKey(hKey);
        return (res == ERROR_SUCCESS);
    }
    return false;
}

static void set_startup_enabled(bool enable) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH] = {0};
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            std::wstring cmd = L"\"" + std::wstring(exePath) + L"\" --tray";
            RegSetValueExW(hKey, L"Fissio", 0, REG_SZ, (const BYTE*)cmd.c_str(), (DWORD)((cmd.length() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, L"Fissio");
        }
        RegCloseKey(hKey);
    }
}

static bool apply_conf_file(const std::wstring& wpath, bool show_error_msg = false) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, NULL, 0, NULL, NULL);
    std::string path_str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, &path_str[0], size_needed, NULL, NULL);

    auto parsed = fissio::WireGuardConfigParser::parse_file(path_str);
    if (parsed.has_value()) {
        g_wgConfig = parsed.value();
        g_hasWgConfig = true;
        g_confPath = path_str;

        save_last_conf_path(wpath);

        std::wstring filename = wpath;
        size_t slash = filename.find_last_of(L"\\/");
        if (slash != std::wstring::npos) filename = filename.substr(slash + 1);

        std::wstring ep_w = utf8_to_wstring(g_wgConfig.endpoint_host);
        if (g_hLblWgConf) {
            SetWindowTextW(g_hLblWgConf, (fissio::tr().lbl_conf_selected_prefix + filename + L" (" + ep_w + L")").c_str());
        }
        if (g_hRadWg) {
            SendMessageW(g_hRadWg, BM_SETCHECK, BST_CHECKED, 0);
            SendMessageW(g_hRadSocks, BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessageW(g_hRadHybrid, BM_SETCHECK, BST_UNCHECKED, 0);
        }
        append_log_w(fissio::tr().log_loaded_conf + filename + L" [Endpoint: " + ep_w + L"]");
        return true;
    } else {
        if (show_error_msg) {
            MessageBoxW(g_hWnd, fissio::tr().dlg_conf_error_body.c_str(), fissio::tr().dlg_conf_error_title.c_str(), MB_ICONERROR);
        }
        return false;
    }
}

// -------------------------------------------------------------
// System Tray Icon & Balloon Notification Updates
// -------------------------------------------------------------
static void update_tray_status(bool is_connected, const std::wstring& detail = L"") {
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    if (is_connected) {
        std::wstring tip = L"FissioWG - Connected";
        if (!detail.empty()) tip += L" (" + detail + L")";
        wcsncpy_s(g_nid.szTip, tip.c_str(), 127);

        wcscpy_s(g_nid.szInfoTitle, fissio::tr().balloon_connected_title.c_str());
        std::wstring info = detail + fissio::tr().balloon_connected_info;
        wcsncpy_s(g_nid.szInfo, info.c_str(), 255);
        g_nid.dwInfoFlags = NIIF_INFO;
    } else {
        wcscpy_s(g_nid.szTip, L"FissioWG - Disconnected");
        wcscpy_s(g_nid.szInfoTitle, fissio::tr().balloon_disconnected_title.c_str());
        wcscpy_s(g_nid.szInfo, fissio::tr().balloon_disconnected_info.c_str());
        g_nid.dwInfoFlags = NIIF_INFO;
    }
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

static void draw_custom_button(DRAWITEMSTRUCT* dis) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    UINT ctrlId = dis->CtlID;
    bool isPressed = (dis->itemState & ODS_SELECTED);
    bool isHovered = (GetPropW(dis->hwndItem, L"HOVER") != NULL);

    int radius = 8;
    COLORREF bgColor = COLOR_CARD_BG;
    COLORREF borderColor = COLOR_CARD_BORDER;
    COLORREF textColor = COLOR_TEXT_WHITE;
    HFONT hFont = g_hFontNormal;

    bool proxy_running = fissio::ProxyController::instance().is_running();
    bool wg_running = fissio::WireGuardTunnel::instance().is_running();
    bool running = proxy_running || wg_running;

    if (ctrlId == IDC_BTN_TOGGLE) {
        radius = 10;
        hFont = g_hFontBtn;
        if (running) {
            bgColor = isPressed ? COLOR_CRIMSON_DRK : (isHovered ? COLOR_CRIMSON_LGT : COLOR_CRIMSON);
            borderColor = bgColor;
            textColor = RGB(255, 255, 255);
        } else {
            bgColor = isPressed ? COLOR_EMERALD_DRK : (isHovered ? COLOR_EMERALD_LGT : COLOR_EMERALD);
            borderColor = bgColor;
            textColor = RGB(255, 255, 255);
        }
    } else if (ctrlId == IDC_BTN_CLEANUP) {
        radius = 8;
        hFont = g_hFontBtn;
        if (isHovered) {
            bgColor = RGB(49, 46, 129);
            borderColor = RGB(129, 140, 248);
            textColor = RGB(255, 255, 255);
        } else {
            bgColor = COLOR_CARD_BG;
            borderColor = COLOR_INDIGO;
            textColor = RGB(224, 231, 255);
        }
    } else if (ctrlId == IDC_BTN_LOAD_CONF) {
        radius = 6;
        hFont = g_hFontNormal;
        bgColor = isHovered ? RGB(51, 65, 85) : RGB(30, 41, 59);
        borderColor = isHovered ? RGB(148, 163, 184) : RGB(71, 85, 105);
        textColor = RGB(241, 245, 249);
    } else if (ctrlId == IDC_BTN_CLEAR) {
        radius = 6;
        hFont = g_hFontNormal;
        bgColor = isHovered ? RGB(51, 65, 85) : RGB(30, 41, 59);
        borderColor = isHovered ? RGB(148, 163, 184) : RGB(71, 85, 105);
        textColor = isHovered ? RGB(241, 245, 249) : RGB(148, 163, 184);
    } else if (ctrlId == IDC_BTN_LANG_DLG || ctrlId == IDOK) {
        radius = 6;
        hFont = g_hFontNormal;
        bgColor = isHovered ? RGB(51, 65, 85) : RGB(30, 41, 59);
        borderColor = isHovered ? RGB(148, 163, 184) : RGB(71, 85, 105);
        textColor = RGB(248, 250, 252);
    }

    HBRUSH hBr = CreateSolidBrush(bgColor);
    HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldBr = SelectObject(hdc, hBr);
    HGDIOBJ oldPen = SelectObject(hdc, hPen);

    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius * 2, radius * 2);

    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPen);
    DeleteObject(hBr);
    DeleteObject(hPen);

    wchar_t text[256] = {0};
    GetWindowTextW(dis->hwndItem, text, 256);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);
    HGDIOBJ oldFont = SelectObject(hdc, hFont);

    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
}

static void update_ui_status() {
    bool proxy_running = fissio::ProxyController::instance().is_running();
    bool wg_running = fissio::WireGuardTunnel::instance().is_running();
    bool running = proxy_running || wg_running;

    if (running) {
        std::wstring epDetail = g_wgConfig.endpoint_host.empty() ? L"Connected" : utf8_to_wstring(g_wgConfig.endpoint_host);

        if (wg_running) {
            SetWindowTextW(g_hLblStatus, (fissio::tr().status_connected_wg + epDetail + L")").c_str());
        } else {
            SetWindowTextW(g_hLblStatus, fissio::tr().status_connected_proxy.c_str());
        }
        SetWindowTextW(g_hBtnToggle, fissio::tr().btn_stop.c_str());
        EnableWindow(g_hEdtPort, FALSE);
        EnableWindow(g_hEdtSplit, FALSE);
        EnableWindow(g_hEdtDelay, FALSE);
        EnableWindow(g_hChkSysProxy, FALSE);
        EnableWindow(g_hRadSocks, FALSE);
        EnableWindow(g_hRadWg, FALSE);
        EnableWindow(g_hRadHybrid, FALSE);
        EnableWindow(g_hBtnLoadConf, FALSE);

        update_tray_status(true, wg_running ? epDetail : L"SOCKS5");
    } else {
        SetWindowTextW(g_hLblStatus, fissio::tr().status_disconnected.c_str());
        SetWindowTextW(g_hBtnToggle, fissio::tr().btn_start.c_str());
        EnableWindow(g_hEdtPort, TRUE);
        EnableWindow(g_hEdtSplit, TRUE);
        EnableWindow(g_hEdtDelay, TRUE);
        EnableWindow(g_hChkSysProxy, TRUE);
        EnableWindow(g_hRadSocks, TRUE);
        EnableWindow(g_hRadWg, TRUE);
        EnableWindow(g_hRadHybrid, TRUE);
        EnableWindow(g_hBtnLoadConf, TRUE);

        update_tray_status(false);
    }

    if (g_hBtnToggle) InvalidateRect(g_hBtnToggle, NULL, TRUE);
    if (g_hWnd) {
        RECT rcStatus{24, 46, 524, 80};
        InvalidateRect(g_hWnd, &rcStatus, TRUE);
    }
}

static void on_load_conf_click() {
    wchar_t szFile[MAX_PATH] = {0};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hWnd;
    ofn.lpstrFilter = L"WG Config (*.conf)\0*.conf\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        apply_conf_file(szFile, true);
    }
}

static void on_toggle_click() {
    auto& proxy_ctl = fissio::ProxyController::instance();
    auto& wg_tunnel = fissio::WireGuardTunnel::instance();

    if (proxy_ctl.is_running() || wg_tunnel.is_running()) {
        proxy_ctl.stop();
        wg_tunnel.stop();
        append_log_w(fissio::tr().log_service_stopped);
    } else {
        bool is_wg = (SendMessageW(g_hRadWg, BM_GETCHECK, 0, 0) == BST_CHECKED);
        bool is_hybrid = (SendMessageW(g_hRadHybrid, BM_GETCHECK, 0, 0) == BST_CHECKED);

        if (is_wg || is_hybrid) {
            if (!g_hasWgConfig || g_confPath.empty()) {
                MessageBoxW(g_hWnd,
                    fissio::tr().dlg_wg_need_conf_body.c_str(),
                    fissio::tr().dlg_wg_need_conf_title.c_str(), MB_ICONINFORMATION);
                on_load_conf_click();
                if (!g_hasWgConfig || g_confPath.empty()) {
                    append_log_w(fissio::tr().log_wg_cancelled);
                    return;
                }
            }

            if (wg_tunnel.start(g_wgConfig, g_confPath)) {
                append_log_w(fissio::tr().log_wg_started);
            } else {
                append_log_w(fissio::tr().log_wg_failed);
            }

            if (is_hybrid) {
                wchar_t buf[64];
                GetWindowTextW(g_hEdtPort, buf, sizeof(buf) / sizeof(wchar_t));
                uint16_t port = static_cast<uint16_t>(_wtoi(buf));
                if (port == 0) port = 10808;

                auto& adv = fissio::AdvancedConfigManager::instance().settings();
                fissio::FragmentConfig cfg;
                cfg.split_size = adv.split_size > 0 ? adv.split_size : 5;
                cfg.delay_ms = adv.delay_ms; // Fully supports 0ms zero-delay
                cfg.enable_fragment = adv.enable_fragment;
                proxy_ctl.start(port, cfg, false);
                append_log_w(fissio::tr().log_hybrid_active + L" (Port: " + std::to_wstring(port) + 
                             L", Delay: " + std::to_wstring(cfg.delay_ms) + L"ms)");
            }
        } else {
            // SOCKS5 proxy mode
            wchar_t buf[64];
            GetWindowTextW(g_hEdtPort, buf, sizeof(buf) / sizeof(wchar_t));
            uint16_t port = static_cast<uint16_t>(_wtoi(buf));
            if (port == 0) port = 10808;

            GetWindowTextW(g_hEdtSplit, buf, sizeof(buf) / sizeof(wchar_t));
            size_t split = _wtoi(buf);
            if (split == 0) split = 5;

            GetWindowTextW(g_hEdtDelay, buf, sizeof(buf) / sizeof(wchar_t));
            int delay = _wtoi(buf);

            bool sysproxy = (SendMessageW(g_hChkSysProxy, BM_GETCHECK, 0, 0) == BST_CHECKED);

            auto& adv = fissio::AdvancedConfigManager::instance().settings();
            fissio::FragmentConfig cfg;
            cfg.split_size = split;
            cfg.delay_ms = delay;
            cfg.enable_fragment = adv.enable_fragment;

            if (proxy_ctl.start(port, cfg, sysproxy)) {
                append_log_w(fissio::tr().log_socks_active + L" (127.0.0.1:" + std::to_wstring(port) + 
                             L", Delay: " + std::to_wstring(delay) + L"ms)");
            } else {
                append_log_w(fissio::tr().log_service_stopped);
            }
        }
    }
    update_ui_status();
}

// -------------------------------------------------------------
// [Emergency] Route Cleanup & Local Network Restoration (Clean-up)
// -------------------------------------------------------------
static int kill_other_fissio_processes() {
    DWORD currentPid = GetCurrentProcessId();
    int killedCount = 0;

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe32{};
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnap, &pe32)) {
        do {
            if (pe32.th32ProcessID == currentPid) {
                continue; // Do not kill the current active GUI instance
            }

            std::wstring procName = pe32.szExeFile;
            for (auto& c : procName) c = towlower(c);

            if (procName == L"fissio-gui.exe" || 
                procName == L"fissio.exe" || 
                procName == L"fissio-wg.exe") {
                
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    CloseHandle(hProc);
                    killedCount++;
                }
            }
        } while (Process32NextW(hSnap, &pe32));
    }
    CloseHandle(hSnap);
    return killedCount;
}

static void refresh_system_tray() {
    // 1. Taskbar Notification Area
    HWND hTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
    if (hTrayWnd) {
        HWND hTrayNotify = FindWindowExW(hTrayWnd, NULL, L"TrayNotifyWnd", NULL);
        if (hTrayNotify) {
            HWND hSysPager = FindWindowExW(hTrayNotify, NULL, L"SysPager", NULL);
            HWND hToolbar = FindWindowExW(hSysPager ? hSysPager : hTrayNotify, NULL, L"ToolbarWindow32", NULL);
            if (hToolbar) {
                RECT rc{};
                GetClientRect(hToolbar, &rc);
                for (int x = 0; x < rc.right; x += 8) {
                    for (int y = 0; y < rc.bottom; y += 8) {
                        SendMessageW(hToolbar, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
                    }
                }
            }
        }
    }

    // 2. Overflow Notification Area (Chevron Flyout)
    HWND hOverflow = FindWindowW(L"NotifyIconOverflowWindow", NULL);
    if (hOverflow) {
        HWND hToolbar = FindWindowExW(hOverflow, NULL, L"ToolbarWindow32", NULL);
        if (hToolbar) {
            RECT rc{};
            GetClientRect(hToolbar, &rc);
            for (int x = 0; x < rc.right; x += 8) {
                for (int y = 0; y < rc.bottom; y += 8) {
                    SendMessageW(hToolbar, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
                }
            }
        }
    }
}

static void execute_emergency_cleanup() {
    append_log_w(L"--------------------------------------------------");
    append_log_w(fissio::tr().log_cleanup_start);

    // 1. Stop active services in current process
    fissio::ProxyController::instance().stop();
    fissio::WireGuardTunnel::instance().stop();

    // 2. Terminate all other Fissio processes (other GUI instances in tray, CLI, WireGuard engines)
    int killed = kill_other_fissio_processes();
    if (killed > 0) {
        append_log_w(fissio::tr().log_cleanup_killed + std::to_wstring(killed));
    } else {
        append_log_w(fissio::tr().log_cleanup_single);
    }
    system("taskkill /F /IM fissio-wg.exe >nul 2>&1");

    // 3. Clear ghost tray icons
    refresh_system_tray();
    append_log_w(fissio::tr().log_cleanup_tray);

    // 4. Remove tunnel bypass routes
    system("route delete 0.0.0.0 mask 128.0.0.0 >nul 2>&1");
    system("route delete 128.0.0.0 mask 128.0.0.0 >nul 2>&1");

    // 5. Remove server endpoint exemption route
    if (!g_wgConfig.endpoint_host.empty()) {
        std::string cmdDelEp = "route delete " + g_wgConfig.endpoint_host + " >nul 2>&1";
        system(cmdDelEp.c_str());
    }

    // 6. Disable Windows system proxy (restore direct internet)
    fissio::SystemProxy::disable();
    append_log_w(fissio::tr().log_cleanup_proxy);

    // 7. Synchronize UI and tray status
    update_ui_status();
    append_log_w(fissio::tr().log_cleanup_done);
    append_log_w(L"--------------------------------------------------");

    std::wstring msg = fissio::tr().dlg_cleanup_success_p1;
    if (killed > 0) {
        msg += std::to_wstring(killed) + fissio::tr().dlg_cleanup_success_p2;
    } else {
        msg = fissio::tr().log_cleanup_single + L"\n\n";
    }
    msg += fissio::tr().dlg_cleanup_p2;
    msg += fissio::tr().dlg_cleanup_p3;
    msg += fissio::tr().dlg_cleanup_p4;
    msg += fissio::tr().dlg_cleanup_footer;

    MessageBoxW(g_hWnd, msg.c_str(), fissio::tr().dlg_cleanup_title.c_str(), MB_ICONINFORMATION);
}

static HICON get_app_icon(HINSTANCE hInstance) {
    HICON hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    if (hIcon) return hIcon;

    hIcon = (HICON)LoadImageW(NULL, L"fissio_gui.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
    if (hIcon) return hIcon;

    hIcon = (HICON)LoadImageW(NULL, L"../fissio_gui.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
    if (hIcon) return hIcon;

    return LoadIconW(NULL, IDI_APPLICATION);
}

static void init_tray_icon(HWND hwnd) {
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = get_app_icon(GetModuleHandleW(NULL));
    wcscpy_s(g_nid.szTip, L"FissioWG - Disconnected");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void show_tray_menu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();

    bool proxy_running = fissio::ProxyController::instance().is_running();
    bool wg_running = fissio::WireGuardTunnel::instance().is_running();
    bool running = proxy_running || wg_running;

    // 0: Status display (disabled item)
    std::wstring statusStr = running ? fissio::tr().tray_status_connected : fissio::tr().tray_status_disconnected;
    if (running) {
        if (wg_running) {
            std::string ep = g_wgConfig.endpoint_host.empty() ? "Connected" : g_wgConfig.endpoint_host;
            statusStr += utf8_to_wstring(ep) + L")";
        } else {
            statusStr += L"SOCKS5 ECH)";
        }
    }
    InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, ID_TRAY_STATUS, statusStr.c_str());
    InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    // 2: Toggle item (Connect / Disconnect)
    InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_TRAY_TOGGLE, running ? fissio::tr().tray_toggle_disconnect.c_str() : fissio::tr().tray_toggle_connect.c_str());
    // 3: Emergency Route Clean-up
    InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_STRING, ID_TRAY_CLEANUP, fissio::tr().tray_cleanup.c_str());
    InsertMenuW(hMenu, 4, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    // 5: Open Window
    InsertMenuW(hMenu, 5, MF_BYPOSITION | MF_STRING, ID_TRAY_OPEN, fissio::tr().tray_open.c_str());
    
    // 6: Language Submenu
    HMENU hLangMenu = CreatePopupMenu();
    auto curLang = fissio::I18nManager::instance().current_language();
    AppendMenuW(hLangMenu, MF_STRING, ID_TRAY_LANG_DIALOG, fissio::tr().tray_lang_dialog.c_str());
    AppendMenuW(hLangMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hLangMenu, MF_STRING | (curLang == fissio::Language::Korean ? MF_CHECKED : 0), ID_TRAY_LANG_KO, L"🇰🇷 한국어");
    AppendMenuW(hLangMenu, MF_STRING | (curLang == fissio::Language::English ? MF_CHECKED : 0), ID_TRAY_LANG_EN, L"🇺🇸 English");
    AppendMenuW(hLangMenu, MF_STRING | (curLang == fissio::Language::Chinese ? MF_CHECKED : 0), ID_TRAY_LANG_ZH, L"🇨🇳 简体中文");
    AppendMenuW(hLangMenu, MF_STRING | (curLang == fissio::Language::Japanese ? MF_CHECKED : 0), ID_TRAY_LANG_JA, L"🇯🇵 日本語");
    AppendMenuW(hLangMenu, MF_STRING | (curLang == fissio::Language::Spanish ? MF_CHECKED : 0), ID_TRAY_LANG_ES, L"🇪🇸 Español");
    AppendMenuW(hLangMenu, MF_STRING | (curLang == fissio::Language::Russian ? MF_CHECKED : 0), ID_TRAY_LANG_RU, L"🇷🇺 Русский");
    AppendMenuW(hLangMenu, MF_STRING | (curLang == fissio::Language::Arabic ? MF_CHECKED : 0), ID_TRAY_LANG_AR, L"🇸🇦 العربية");
    InsertMenuW(hMenu, 6, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hLangMenu, L"🌐 Language / 언어 설정");

    InsertMenuW(hMenu, 7, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    // 8: Exit
    InsertMenuW(hMenu, 8, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, fissio::tr().tray_exit.c_str());

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

// -------------------------------------------------------------
// Real-time Language Selection Dialog Window ("실시간 언어 변경창")
// -------------------------------------------------------------
static HWND g_hDlgRadio[7] = {NULL};
static HWND g_hDlgTitle = NULL;
static HWND g_hDlgDesc = NULL;
static HWND g_hDlgClose = NULL;

static void update_lang_dialog_texts() {
    if (!g_hLangDlg) return;
    const auto& t = fissio::tr();
    SetWindowTextW(g_hLangDlg, t.dlg_lang_title.c_str());
    if (g_hDlgTitle) SetWindowTextW(g_hDlgTitle, t.dlg_lang_title.c_str());
    if (g_hDlgDesc) SetWindowTextW(g_hDlgDesc, t.dlg_lang_desc.c_str());
    if (g_hDlgClose) SetWindowTextW(g_hDlgClose, t.dlg_lang_close.c_str());

    auto curLang = fissio::I18nManager::instance().current_language();
    for (int i = 0; i < 7; ++i) {
        if (g_hDlgRadio[i]) {
            SendMessageW(g_hDlgRadio[i], BM_SETCHECK, (i == (int)curLang) ? BST_CHECKED : BST_UNCHECKED, 0);
        }
    }
}

static void apply_language(fissio::Language lang);

static LRESULT CALLBACK LangDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        apply_dark_window_attributes(hwnd);
        const auto& t = fissio::tr();
        g_hDlgTitle = CreateWindowW(L"STATIC", t.dlg_lang_title.c_str(),
                                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                                    24, 16, 340, 26, hwnd, NULL, g_hInst, NULL);
        SendMessageW(g_hDlgTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

        g_hDlgDesc = CreateWindowW(L"STATIC", t.dlg_lang_desc.c_str(),
                                   WS_CHILD | WS_VISIBLE | SS_LEFT,
                                   24, 48, 340, 42, hwnd, NULL, g_hInst, NULL);
        SendMessageW(g_hDlgDesc, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        const wchar_t* lang_names[7] = {
            L"🇰🇷 한국어 (Korean)",
            L"🇺🇸 English (영어)",
            L"🇨🇳 简体中文 (중국어)",
            L"🇯🇵 日本語 (일본어)",
            L"🇪🇸 Español (스페인어)",
            L"🇷🇺 Русский (러시아어)",
            L"🇸🇦 العربية (아랍어)"
        };

        auto curLang = fissio::I18nManager::instance().current_language();
        for (int i = 0; i < 7; ++i) {
            DWORD style = WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | ((i == 0) ? WS_GROUP : 0);
            g_hDlgRadio[i] = CreateWindowW(L"BUTTON", lang_names[i], style,
                                          32, 94 + i * 32, 320, 26,
                                          hwnd, (HMENU)(UINT_PTR)(3000 + i), g_hInst, NULL);
            SendMessageW(g_hDlgRadio[i], WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SetWindowTheme(g_hDlgRadio[i], L"DarkMode_Explorer", NULL);
            if (i == (int)curLang) {
                SendMessageW(g_hDlgRadio[i], BM_SETCHECK, BST_CHECKED, 0);
            }
        }

        g_hDlgClose = CreateWindowW(L"BUTTON", t.dlg_lang_close.c_str(),
                                   WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                   125, 332, 140, 36, hwnd, (HMENU)IDOK, g_hInst, NULL);
        SendMessageW(g_hDlgClose, WM_SETFONT, (WPARAM)g_hFontBtn, TRUE);
        SetWindowSubclass(g_hDlgClose, ModernBtnSubclass, IDOK, 0);
        return 0;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        if (wmId >= 3000 && wmId <= 3006) {
            fissio::Language selected = static_cast<fissio::Language>(wmId - 3000);
            apply_language(selected);
        } else if (wmId == IDOK || wmId == IDCANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
        if (dis && dis->CtlType == ODT_BUTTON) {
            draw_custom_button(dis);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_TEXT_WHITE);
        SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)g_hBrushBg;
    }
    case WM_CTLCOLORDLG:
        return (INT_PTR)g_hBrushBg;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (g_hWnd) {
            EnableWindow(g_hWnd, TRUE);
            SetForegroundWindow(g_hWnd);
        }
        g_hLangDlg = NULL;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void show_language_dialog(HWND hParent) {
    if (g_hLangDlg) {
        SetForegroundWindow(g_hLangDlg);
        return;
    }

    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = LangDlgProc;
        wc.hInstance = g_hInst;
        wc.hIcon = get_app_icon(g_hInst);
        wc.hIconSm = get_app_icon(g_hInst);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(COLOR_BG);
        wc.lpszClassName = L"FissioLangDialogClass";
        RegisterClassExW(&wc);
        classRegistered = true;
    }

    RECT rcParent{};
    GetWindowRect(hParent, &rcParent);
    int dlgW = 400;
    int dlgH = 425;
    int dlgX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int dlgY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;

    g_hLangDlg = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
                                 L"FissioLangDialogClass",
                                 fissio::tr().dlg_lang_title.c_str(),
                                 WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                                 dlgX, dlgY, dlgW, dlgH, hParent, NULL, g_hInst, NULL);

    if (g_hLangDlg) {
        apply_dark_window_attributes(g_hLangDlg);
        EnableWindow(hParent, FALSE);
        ShowWindow(g_hLangDlg, SW_SHOW);
        SetForegroundWindow(g_hLangDlg);
    }
}

static void apply_language(fissio::Language lang) {
    fissio::I18nManager::instance().set_language(lang);
    const auto& t = fissio::tr();

    if (g_hCmbLang) {
        SendMessageW(g_hCmbLang, CB_SETCURSEL, (WPARAM)lang, 0);
    }

    if (g_hWnd) SetWindowTextW(g_hWnd, t.window_title.c_str());
    if (g_hTitle) SetWindowTextW(g_hTitle, t.header_title.c_str());
    if (g_hBtnLangDlg) SetWindowTextW(g_hBtnLangDlg, t.btn_lang_dialog.c_str());
    if (g_hGrpMode) SetWindowTextW(g_hGrpMode, t.grp_mode.c_str());
    if (g_hRadSocks) SetWindowTextW(g_hRadSocks, t.rad_socks5.c_str());
    if (g_hRadWg) SetWindowTextW(g_hRadWg, t.rad_wg.c_str());
    if (g_hRadHybrid) SetWindowTextW(g_hRadHybrid, t.rad_hybrid.c_str());
    if (g_hBtnLoadConf) SetWindowTextW(g_hBtnLoadConf, t.btn_load_conf.c_str());

    if (g_hLblWgConf) {
        if (g_hasWgConfig) {
            std::wstring filename = utf8_to_wstring(g_confPath);
            size_t slash = filename.find_last_of(L"\\/");
            if (slash != std::wstring::npos) filename = filename.substr(slash + 1);
            std::wstring ep_w = utf8_to_wstring(g_wgConfig.endpoint_host);
            SetWindowTextW(g_hLblWgConf, (t.lbl_conf_selected_prefix + filename + L" (" + ep_w + L")").c_str());
        } else {
            SetWindowTextW(g_hLblWgConf, t.lbl_conf_default.c_str());
        }
    }

    if (g_hGrpCfg) SetWindowTextW(g_hGrpCfg, t.grp_config.c_str());
    if (g_hLblPort) SetWindowTextW(g_hLblPort, t.lbl_port.c_str());
    if (g_hLblSplit) SetWindowTextW(g_hLblSplit, t.lbl_split.c_str());
    if (g_hLblDelay) SetWindowTextW(g_hLblDelay, t.lbl_delay.c_str());
    if (g_hChkSysProxy) SetWindowTextW(g_hChkSysProxy, t.chk_sysproxy.c_str());
    if (g_hChkStartup) SetWindowTextW(g_hChkStartup, t.chk_startup.c_str());
    if (g_hGrpLog) SetWindowTextW(g_hGrpLog, t.grp_log.c_str());
    if (g_hBtnClear) SetWindowTextW(g_hBtnClear, t.btn_clear_log.c_str());
    if (g_hBtnCleanup) SetWindowTextW(g_hBtnCleanup, t.btn_cleanup.c_str());

    update_ui_status();
    update_lang_dialog_texts();

    if (g_hWnd) {
        InvalidateRect(g_hWnd, NULL, TRUE);
        UpdateWindow(g_hWnd);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        apply_dark_window_attributes(hwnd);

        // Universal fonts using DEFAULT_CHARSET for multi-language rendering
        g_hFontNormal = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontTitle = CreateFontW(19, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontStatus = CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontBtn = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontSection = CreateFontW(13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontConsole = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

        g_hBrushBg = CreateSolidBrush(COLOR_BG);
        g_hBrushCard = CreateSolidBrush(COLOR_CARD_BG);
        g_hBrushInput = CreateSolidBrush(COLOR_INPUT_BG);
        g_hPenCardBorder = CreatePen(PS_SOLID, 1, COLOR_CARD_BORDER);
        g_hPenInputBorder = CreatePen(PS_SOLID, 1, COLOR_INPUT_BORDER);

        const auto& t = fissio::tr();

        // 1. Header Title & Language Controls
        g_hTitle = CreateWindowW(L"STATIC", t.header_title.c_str(),
                                 WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                                 24, 16, 310, 26, hwnd, NULL, g_hInst, NULL);
        SendMessageW(g_hTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

        g_hCmbLang = CreateWindowW(L"COMBOBOX", NULL,
                                  WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                  336, 14, 114, 260, hwnd, (HMENU)IDC_CMB_LANG, g_hInst, NULL);
        SendMessageW(g_hCmbLang, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        SendMessageW(g_hCmbLang, CB_ADDSTRING, 0, (LPARAM)fissio::I18nManager::get_language_name(fissio::Language::Korean));
        SendMessageW(g_hCmbLang, CB_ADDSTRING, 0, (LPARAM)fissio::I18nManager::get_language_name(fissio::Language::English));
        SendMessageW(g_hCmbLang, CB_ADDSTRING, 0, (LPARAM)fissio::I18nManager::get_language_name(fissio::Language::Chinese));
        SendMessageW(g_hCmbLang, CB_ADDSTRING, 0, (LPARAM)fissio::I18nManager::get_language_name(fissio::Language::Japanese));
        SendMessageW(g_hCmbLang, CB_ADDSTRING, 0, (LPARAM)fissio::I18nManager::get_language_name(fissio::Language::Spanish));
        SendMessageW(g_hCmbLang, CB_ADDSTRING, 0, (LPARAM)fissio::I18nManager::get_language_name(fissio::Language::Russian));
        SendMessageW(g_hCmbLang, CB_ADDSTRING, 0, (LPARAM)fissio::I18nManager::get_language_name(fissio::Language::Arabic));
        SendMessageW(g_hCmbLang, CB_SETCURSEL, (WPARAM)fissio::I18nManager::instance().current_language(), 0);

        g_hBtnLangDlg = CreateWindowW(L"BUTTON", t.btn_lang_dialog.c_str(),
                                     WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                     456, 14, 68, 27, hwnd, (HMENU)IDC_BTN_LANG_DLG, g_hInst, NULL);
        SendMessageW(g_hBtnLangDlg, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SetWindowSubclass(g_hBtnLangDlg, ModernBtnSubclass, IDC_BTN_LANG_DLG, 0);

        // 2. Status Label (placed inside status banner card)
        g_hLblStatus = CreateWindowW(L"STATIC", t.status_disconnected.c_str(),
                                    WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                                    60, 52, 450, 22, hwnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblStatus, WM_SETFONT, (WPARAM)g_hFontStatus, TRUE);

        // 3. Main Toggle Hero Button
        g_hBtnToggle = CreateWindowW(L"BUTTON", t.btn_start.c_str(),
                                    WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                    24, 88, 500, 48, hwnd, (HMENU)IDC_BTN_TOGGLE, g_hInst, NULL);
        SendMessageW(g_hBtnToggle, WM_SETFONT, (WPARAM)g_hFontBtn, TRUE);
        SetWindowSubclass(g_hBtnToggle, ModernBtnSubclass, IDC_BTN_TOGGLE, 0);

        // 4. Card 1 Controls (Operation Mode)
        g_hRadSocks = CreateWindowW(L"BUTTON", t.rad_socks5.c_str(),
                                   WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                                   38, 172, 145, 24, hwnd, (HMENU)IDC_RAD_SOCKS5, g_hInst, NULL);
        SendMessageW(g_hRadSocks, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        g_hRadWg = CreateWindowW(L"BUTTON", t.rad_wg.c_str(),
                                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                192, 172, 145, 24, hwnd, (HMENU)IDC_RAD_WG, g_hInst, NULL);
        SendMessageW(g_hRadWg, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        g_hRadHybrid = CreateWindowW(L"BUTTON", t.rad_hybrid.c_str(),
                                    WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                    346, 172, 160, 24, hwnd, (HMENU)IDC_RAD_HYBRID, g_hInst, NULL);
        SendMessageW(g_hRadHybrid, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        g_hBtnLoadConf = CreateWindowW(L"BUTTON", t.btn_load_conf.c_str(),
                                      WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                      38, 202, 150, 26, hwnd, (HMENU)IDC_BTN_LOAD_CONF, g_hInst, NULL);
        SendMessageW(g_hBtnLoadConf, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SetWindowSubclass(g_hBtnLoadConf, ModernBtnSubclass, IDC_BTN_LOAD_CONF, 0);

        g_hLblWgConf = CreateWindowW(L"STATIC", t.lbl_conf_default.c_str(),
                                    WS_CHILD | WS_VISIBLE | SS_LEFT | SS_PATHELLIPSIS,
                                    196, 205, 315, 20, hwnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblWgConf, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

        // 5. Card 2 Controls (Configuration & Toggles)
        g_hLblPort = CreateWindowW(L"STATIC", t.lbl_port.c_str(), WS_CHILD | WS_VISIBLE, 38, 274, 68, 20, hwnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblPort, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        g_hEdtPort = CreateWindowW(L"EDIT", L"10808", WS_CHILD | WS_VISIBLE | ES_NUMBER,
                                  108, 272, 58, 20, hwnd, (HMENU)IDC_EDT_PORT, g_hInst, NULL);
        SendMessageW(g_hEdtPort, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SetWindowTheme(g_hEdtPort, L"DarkMode_Explorer", NULL);

        g_hLblSplit = CreateWindowW(L"STATIC", t.lbl_split.c_str(), WS_CHILD | WS_VISIBLE, 185, 274, 85, 20, hwnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblSplit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        g_hEdtSplit = CreateWindowW(L"EDIT", L"5", WS_CHILD | WS_VISIBLE | ES_NUMBER,
                                   275, 272, 45, 20, hwnd, (HMENU)IDC_EDT_SPLIT, g_hInst, NULL);
        SendMessageW(g_hEdtSplit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SetWindowTheme(g_hEdtSplit, L"DarkMode_Explorer", NULL);

        g_hLblDelay = CreateWindowW(L"STATIC", t.lbl_delay.c_str(), WS_CHILD | WS_VISIBLE, 335, 274, 65, 20, hwnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblDelay, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        g_hEdtDelay = CreateWindowW(L"EDIT", L"0", WS_CHILD | WS_VISIBLE | ES_NUMBER,
                                   405, 272, 45, 20, hwnd, (HMENU)IDC_EDT_DELAY, g_hInst, NULL);
        SendMessageW(g_hEdtDelay, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SetWindowTheme(g_hEdtDelay, L"DarkMode_Explorer", NULL);

        g_hChkSysProxy = CreateWindowW(L"BUTTON", t.chk_sysproxy.c_str(),
                                      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                      38, 304, 470, 22, hwnd, (HMENU)IDC_CHK_SYSPROXY, g_hInst, NULL);
        SendMessageW(g_hChkSysProxy, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessageW(g_hChkSysProxy, BM_SETCHECK, BST_CHECKED, 0);
        SetWindowTheme(g_hChkSysProxy, L"DarkMode_Explorer", NULL);

        g_hChkStartup = CreateWindowW(L"BUTTON", t.chk_startup.c_str(),
                                     WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                     38, 330, 470, 22, hwnd, (HMENU)IDC_CHK_STARTUP, g_hInst, NULL);
        SendMessageW(g_hChkStartup, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessageW(g_hChkStartup, BM_SETCHECK, is_startup_enabled() ? BST_CHECKED : BST_UNCHECKED, 0);
        SetWindowTheme(g_hChkStartup, L"DarkMode_Explorer", NULL);

        // 6. Card 3 Controls (Realtime Log Console)
        g_hBtnClear = CreateWindowW(L"BUTTON", t.btn_clear_log.c_str(), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                    434, 376, 80, 24, hwnd, (HMENU)IDC_BTN_CLEAR, g_hInst, NULL);
        SendMessageW(g_hBtnClear, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SetWindowSubclass(g_hBtnClear, ModernBtnSubclass, IDC_BTN_CLEAR, 0);

        g_hEdtLog = CreateWindowW(L"EDIT", L"",
                                 WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                                 ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                 38, 406, 472, 158, hwnd, (HMENU)IDC_EDT_LOG, g_hInst, NULL);
        SendMessageW(g_hEdtLog, WM_SETFONT, (WPARAM)g_hFontConsole, TRUE);
        SetWindowTheme(g_hEdtLog, L"DarkMode_Explorer", NULL);

        // Apply dark theme to combobox and radio buttons
        SetWindowTheme(g_hCmbLang, L"DarkMode_Explorer", NULL);
        SetWindowTheme(g_hRadSocks, L"DarkMode_Explorer", NULL);
        SetWindowTheme(g_hRadWg, L"DarkMode_Explorer", NULL);
        SetWindowTheme(g_hRadHybrid, L"DarkMode_Explorer", NULL);

        // Enable Drag-and-Drop file loading (.conf)
        DragAcceptFiles(hwnd, TRUE);

        // 7. Route Clean-up & Emergency Restoration Button
        g_hBtnCleanup = CreateWindowW(L"BUTTON", t.btn_cleanup.c_str(),
                                     WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                     24, 588, 500, 36, hwnd, (HMENU)IDC_BTN_CLEANUP, g_hInst, NULL);
        SendMessageW(g_hBtnCleanup, WM_SETFONT, (WPARAM)g_hFontBtn, TRUE);
        SetWindowSubclass(g_hBtnCleanup, ModernBtnSubclass, IDC_BTN_CLEANUP, 0);

        // Connect log callback (safely convert UTF-8 string to UTF-16 wstring for UI)
        fissio::ProxyController::instance().set_log_callback([](const std::string& msg) {
            std::wstring* pwStr = new std::wstring(utf8_to_wstring(msg));
            PostMessageW(g_hWnd, WM_USER + 10, 0, (LPARAM)pwStr);
        });

        init_tray_icon(hwnd);
        append_log_w(t.log_ready);

        // Automatically reload previous configuration file (Persistence)
        std::wstring lastConf = load_last_conf_path();
        if (!lastConf.empty()) {
            if (apply_conf_file(lastConf, false)) {
                append_log_w(fissio::tr().log_loaded_conf + lastConf);
            }
        } else {
            SendMessageW(g_hRadSocks, BM_SETCHECK, BST_CHECKED, 0);
        }

        return 0;
    }

    case WM_USER + 10: {
        std::wstring* pStr = reinterpret_cast<std::wstring*>(lParam);
        if (pStr) {
            append_log_w(*pStr);
            delete pStr;
        }
        return 0;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_BTN_TOGGLE:
            on_toggle_click();
            break;
        case IDC_BTN_LOAD_CONF:
            on_load_conf_click();
            break;
        case IDC_BTN_CLEAR:
            SetWindowTextW(g_hEdtLog, L"");
            break;
        case IDC_CHK_STARTUP: {
            bool enable = (SendMessageW(g_hChkStartup, BM_GETCHECK, 0, 0) == BST_CHECKED);
            set_startup_enabled(enable);
            if (enable) {
                append_log_w(L"[Auto-Start] Windows startup auto-launch enabled (minimized to tray).");
            } else {
                append_log_w(L"[Auto-Start] Windows startup auto-launch disabled.");
            }
            break;
        }
        case ID_TRAY_LANG_KO:
            apply_language(fissio::Language::Korean);
            break;
        case ID_TRAY_LANG_EN:
            apply_language(fissio::Language::English);
            break;
        case ID_TRAY_LANG_ZH:
            apply_language(fissio::Language::Chinese);
            break;
        case ID_TRAY_LANG_JA:
            apply_language(fissio::Language::Japanese);
            break;
        case ID_TRAY_LANG_ES:
            apply_language(fissio::Language::Spanish);
            break;
        case ID_TRAY_LANG_RU:
            apply_language(fissio::Language::Russian);
            break;
        case ID_TRAY_LANG_AR:
            apply_language(fissio::Language::Arabic);
            break;
        case IDC_CMB_LANG:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = (int)SendMessageW(g_hCmbLang, CB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel <= 6) {
                    apply_language(static_cast<fissio::Language>(sel));
                }
            }
            break;
        case IDC_BTN_LANG_DLG:
        case ID_TRAY_LANG_DIALOG:
            show_language_dialog(hwnd);
            break;
        case IDC_BTN_CLEANUP:
        case ID_TRAY_CLEANUP:
            execute_emergency_cleanup();
            break;
        case ID_TRAY_OPEN:
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
            break;
        case ID_TRAY_TOGGLE:
            on_toggle_click();
            break;
        case ID_TRAY_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        return 0;
    }

    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        wchar_t szFile[MAX_PATH] = {0};
        if (DragQueryFileW(hDrop, 0, szFile, MAX_PATH)) {
            std::wstring filePath = szFile;
            size_t dot = filePath.rfind(L'.');
            if (dot != std::wstring::npos) {
                std::wstring ext = filePath.substr(dot);
                for (auto& c : ext) c = towlower(c);
                if (ext == L".conf") {
                    if (apply_conf_file(szFile, true)) {
                        append_log_w(fissio::tr().log_loaded_conf + szFile);
                        SendMessageW(g_hRadSocks, BM_SETCHECK, BST_UNCHECKED, 0);
                        SendMessageW(g_hRadWg, BM_SETCHECK, BST_CHECKED, 0);
                        SendMessageW(g_hRadHybrid, BM_SETCHECK, BST_UNCHECKED, 0);
                    }
                } else {
                    MessageBoxW(hwnd, fissio::tr().dlg_conf_error_body.c_str(),
                                fissio::tr().dlg_conf_error_title.c_str(), MB_ICONWARNING);
                }
            }
        }
        DragFinish(hDrop);
        return 0;
    }

    case WM_TRAYICON: {
        if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP) {
            if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
                // If window is already visible, bring to foreground
                SetForegroundWindow(hwnd);
            } else {
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
            }
        } else if (lParam == WM_RBUTTONUP) {
            show_tray_menu(hwnd);
        }
        return 0;
    }

    case WM_SYSCOMMAND: {
        if (wParam == SC_MINIMIZE) {
            ShowWindow(hwnd, SW_HIDE);
            if (!g_trayNoticeShown) {
                g_nid.uFlags = NIF_INFO;
                wcsncpy_s(g_nid.szInfoTitle, fissio::tr().balloon_tray_minimize_title.c_str(), 63);
                wcsncpy_s(g_nid.szInfo, fissio::tr().balloon_tray_minimize_info.c_str(), 255);
                g_nid.dwInfoFlags = NIIF_INFO;
                Shell_NotifyIconW(NIM_MODIFY, &g_nid);
                g_trayNoticeShown = true;
            }
            return 0;
        }
        break;
    }

    case WM_CLOSE: {
        // On close (X), hide to system tray instead of terminating process
        ShowWindow(hwnd, SW_HIDE);
        if (!g_trayNoticeShown) {
            g_nid.uFlags = NIF_INFO;
            wcsncpy_s(g_nid.szInfoTitle, fissio::tr().balloon_tray_close_title.c_str(), 63);
            wcsncpy_s(g_nid.szInfo, fissio::tr().balloon_tray_close_info.c_str(), 255);
            g_nid.dwInfoFlags = NIIF_INFO;
            Shell_NotifyIconW(NIM_MODIFY, &g_nid);
            g_trayNoticeShown = true;
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        FillRect(hdc, &rcClient, g_hBrushBg);

        auto draw_card = [&](int x, int y, int w, int h, const std::wstring& header) {
            HGDIOBJ oldBrush = SelectObject(hdc, g_hBrushCard);
            HGDIOBJ oldPen = SelectObject(hdc, g_hPenCardBorder);
            RoundRect(hdc, x, y, x + w, y + h, 14, 14);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);

            if (!header.empty()) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, COLOR_TEXT_ACCENT);
                HGDIOBJ oldFont = SelectObject(hdc, g_hFontSection);
                RECT rcHdr{x + 16, y + 10, x + w - 16, y + 28};
                DrawTextW(hdc, header.c_str(), -1, &rcHdr, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
                SelectObject(hdc, oldFont);
            }
        };

        const auto& t = fissio::tr();

        // 1. Status Hero Banner Card (Top)
        int statusCardX = 24, statusCardY = 46, statusCardW = 500, statusCardH = 34;
        HGDIOBJ oldBrush = SelectObject(hdc, g_hBrushCard);
        HGDIOBJ oldPen = SelectObject(hdc, g_hPenCardBorder);
        RoundRect(hdc, statusCardX, statusCardY, statusCardX + statusCardW, statusCardY + statusCardH, 12, 12);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);

        // Glowing status indicator (Outer soft ring + inner solid dot)
        bool proxy_running = fissio::ProxyController::instance().is_running();
        bool wg_running = fissio::WireGuardTunnel::instance().is_running();
        bool running = proxy_running || wg_running;

        COLORREF dotColor = running ? COLOR_EMERALD : COLOR_CRIMSON;
        COLORREF ringColor = running ? RGB(20, 83, 45) : RGB(88, 28, 28);

        HBRUSH hRingBr = CreateSolidBrush(ringColor);
        HPEN hRingPen = CreatePen(PS_SOLID, 1, running ? COLOR_EMERALD_LGT : COLOR_CRIMSON_LGT);
        HGDIOBJ ob1 = SelectObject(hdc, hRingBr);
        HGDIOBJ op1 = SelectObject(hdc, hRingPen);
        Ellipse(hdc, 38, 55, 53, 70);
        SelectObject(hdc, ob1);
        SelectObject(hdc, op1);
        DeleteObject(hRingBr);
        DeleteObject(hRingPen);

        HBRUSH hDotBr = CreateSolidBrush(dotColor);
        HPEN hDotPen = CreatePen(PS_SOLID, 1, dotColor);
        HGDIOBJ ob2 = SelectObject(hdc, hDotBr);
        HGDIOBJ op2 = SelectObject(hdc, hDotPen);
        Ellipse(hdc, 41, 58, 50, 67);
        SelectObject(hdc, ob2);
        SelectObject(hdc, op2);
        DeleteObject(hDotBr);
        DeleteObject(hDotPen);

        auto draw_input_box = [&](int x, int y, int w, int h, int r) {
            HGDIOBJ oldBrush = SelectObject(hdc, g_hBrushInput);
            HGDIOBJ oldPen = SelectObject(hdc, g_hPenInputBorder);
            RoundRect(hdc, x, y, x + w, y + h, r, r);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
        };

        // 2. Card 1: Operation Mode
        draw_card(24, 144, 500, 94, t.grp_mode);

        // 3. Card 2: Configuration & Toggles
        draw_card(24, 246, 500, 118, t.grp_config);
        draw_input_box(104, 269, 66, 26, 6);
        draw_input_box(271, 269, 53, 26, 6);
        draw_input_box(401, 269, 53, 26, 6);

        // 4. Card 3: Realtime Event Logs
        draw_card(24, 372, 500, 208, t.grp_log);
        draw_input_box(34, 402, 480, 166, 8);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
        if (dis && dis->CtlType == ODT_BUTTON) {
            draw_custom_button(dis);
            return TRUE;
        }
        break;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        if (hCtrl == g_hEdtLog) {
            SetTextColor(hdc, COLOR_EMERALD_LGT);
            SetBkColor(hdc, COLOR_INPUT_BG);
            return (INT_PTR)g_hBrushInput;
        } else {
            SetTextColor(hdc, COLOR_TEXT_WHITE);
            SetBkColor(hdc, COLOR_INPUT_BG);
            return (INT_PTR)g_hBrushInput;
        }
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        if (hCtrl == g_hTitle) {
            SetTextColor(hdc, COLOR_TEXT_WHITE);
            SetBkMode(hdc, TRANSPARENT);
            return (INT_PTR)g_hBrushBg;
        }
        if (hCtrl == g_hLblStatus) {
            bool running = fissio::ProxyController::instance().is_running() ||
                           fissio::WireGuardTunnel::instance().is_running();
            SetTextColor(hdc, running ? COLOR_EMERALD_LGT : RGB(248, 113, 113));
            SetBkMode(hdc, TRANSPARENT);
            return (INT_PTR)g_hBrushCard;
        }
        if (hCtrl == g_hLblPort || hCtrl == g_hLblSplit || hCtrl == g_hLblDelay || hCtrl == g_hLblWgConf) {
            SetTextColor(hdc, COLOR_TEXT_MUTED);
            SetBkMode(hdc, TRANSPARENT);
            return (INT_PTR)g_hBrushCard;
        }
        SetTextColor(hdc, RGB(226, 232, 240));
        SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)g_hBrushCard;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_TEXT_WHITE);
        SetBkColor(hdc, COLOR_CARD_BG);
        return (INT_PTR)g_hBrushCard;
    }

    case WM_CTLCOLORBTN:
        return (INT_PTR)g_hBrushBg;

    case WM_CTLCOLORDLG:
        return (INT_PTR)g_hBrushBg;

    case WM_DESTROY: {
        fissio::ProxyController::instance().stop();
        fissio::WireGuardTunnel::instance().stop();
        system("taskkill /F /IM fissio-wg.exe >nul 2>&1");
        system("route delete 0.0.0.0 mask 128.0.0.0 >nul 2>&1");
        system("route delete 128.0.0.0 mask 128.0.0.0 >nul 2>&1");
        if (!g_wgConfig.endpoint_host.empty()) {
            std::string cmdDel = "route delete " + g_wgConfig.endpoint_host + " >nul 2>&1";
            system(cmdDel.c_str());
        }
        fissio::SystemProxy::disable();
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        if (g_hFontNormal) DeleteObject(g_hFontNormal);
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        if (g_hFontStatus) DeleteObject(g_hFontStatus);
        if (g_hFontBtn) DeleteObject(g_hFontBtn);
        if (g_hFontSection) DeleteObject(g_hFontSection);
        if (g_hFontConsole) DeleteObject(g_hFontConsole);
        if (g_hBrushBg) DeleteObject(g_hBrushBg);
        if (g_hBrushCard) DeleteObject(g_hBrushCard);
        if (g_hBrushInput) DeleteObject(g_hBrushInput);
        if (g_hPenCardBorder) DeleteObject(g_hPenCardBorder);
        if (g_hPenInputBorder) DeleteObject(g_hPenInputBorder);
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, int nCmdShow) {
    // Prevent duplicate instances: restore and focus existing window if already running
    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Global\\FissioGuiSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(L"FissioGuiWindowClass", NULL);
        if (hExisting) {
            ShowWindow(hExisting, SW_SHOW);
            ShowWindow(hExisting, SW_RESTORE);
            SetForegroundWindow(hExisting);
        }
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    g_hInst = hInstance;

    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    HICON hAppIcon = get_app_icon(hInstance);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = hAppIcon;
    wc.hIconSm = hAppIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COLOR_BG);
    wc.lpszClassName = L"FissioGuiWindowClass";
    RegisterClassExW(&wc);

    int width = 565;
    int height = 680;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - width) / 2;
    int y = (screenH - height) / 2;

    g_hWnd = CreateWindowExW(0, L"FissioGuiWindowClass",
                            fissio::tr().window_title.c_str(),
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                            x, y, width, height, NULL, NULL, hInstance, NULL);

    if (!g_hWnd) {
        WSACleanup();
        return 0;
    }

    apply_dark_window_attributes(g_hWnd);

    SendMessageW(g_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hAppIcon);
    SendMessageW(g_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hAppIcon);

    // Start hidden directly in system tray if command line contains --tray, /tray, or -minimized
    bool start_in_tray = false;
    if (lpCmdLine != NULL) {
        std::wstring cmd(lpCmdLine);
        if (cmd.find(L"tray") != std::wstring::npos || cmd.find(L"minimized") != std::wstring::npos) {
            start_in_tray = true;
        }
    }

    if (!start_in_tray) {
        int show_mode = (nCmdShow == SW_HIDE || nCmdShow == 0) ? SW_SHOWNORMAL : nCmdShow;
        ShowWindow(g_hWnd, show_mode);
        UpdateWindow(g_hWnd);
    } else {
        ShowWindow(g_hWnd, SW_HIDE);
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    WSACleanup();
    return (int)msg.wParam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR, int nCmdShow) {
    return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nCmdShow);
}

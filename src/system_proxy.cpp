#include "system_proxy.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <wininet.h>
#include <iostream>

#pragma comment(lib, "wininet.lib")

namespace fissio {

static void notify_internet_settings_change() {
    InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    InternetSetOption(NULL, INTERNET_OPTION_REFRESH, NULL, 0);
}

bool SystemProxy::enable_socks5(uint16_t port) {
    HKEY hKey;
    LONG res = RegOpenKeyExA(HKEY_CURRENT_USER,
                            "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
                            0, KEY_SET_VALUE, &hKey);
    if (res != ERROR_SUCCESS) return false;

    DWORD enable = 1;
    RegSetValueExA(hKey, "ProxyEnable", 0, REG_DWORD, (const BYTE*)&enable, sizeof(enable));

    // Universal proxy configuration covering HTTP, HTTPS, and SOCKS5
    std::string pstr = std::to_string(port);
    std::string proxy_server = "http=127.0.0.1:" + pstr + ";https=127.0.0.1:" + pstr + ";socks=socks5://127.0.0.1:" + pstr;
    RegSetValueExA(hKey, "ProxyServer", 0, REG_SZ, (const BYTE*)proxy_server.c_str(), (DWORD)proxy_server.size() + 1);

    // Bypass proxy for local addresses
    std::string override_str = "<local>;127.0.0.1;localhost;*.local;*.internal";
    RegSetValueExA(hKey, "ProxyOverride", 0, REG_SZ, (const BYTE*)override_str.c_str(), (DWORD)override_str.size() + 1);

    RegCloseKey(hKey);
    notify_internet_settings_change();
    return true;
}

bool SystemProxy::disable() {
    HKEY hKey;
    LONG res = RegOpenKeyExA(HKEY_CURRENT_USER,
                            "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
                            0, KEY_SET_VALUE, &hKey);
    if (res != ERROR_SUCCESS) return false;

    DWORD enable = 0;
    RegSetValueExA(hKey, "ProxyEnable", 0, REG_DWORD, (const BYTE*)&enable, sizeof(enable));

    RegCloseKey(hKey);
    notify_internet_settings_change();
    return true;
}

bool SystemProxy::is_enabled() {
    HKEY hKey;
    LONG res = RegOpenKeyExA(HKEY_CURRENT_USER,
                            "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
                            0, KEY_QUERY_VALUE, &hKey);
    if (res != ERROR_SUCCESS) return false;

    DWORD enable = 0;
    DWORD len = sizeof(enable);
    DWORD type = REG_DWORD;
    res = RegQueryValueExA(hKey, "ProxyEnable", NULL, &type, (LPBYTE)&enable, &len);
    RegCloseKey(hKey);

    return (res == ERROR_SUCCESS && enable == 1);
}

} // namespace fissio
#else
namespace fissio {
bool SystemProxy::enable_socks5(uint16_t) { return false; }
bool SystemProxy::disable() { return false; }
bool SystemProxy::is_enabled() { return false; }
}
#endif

#include "wireguard_tunnel.hpp"
#include "proxy_controller.hpp"
#include <fstream>
#include <iostream>
#include <vector>

#if defined(_WIN32)
#include <windows.h>

namespace fissio {

WireGuardTunnel& WireGuardTunnel::instance() {
    static WireGuardTunnel inst;
    return inst;
}

WireGuardTunnel::WireGuardTunnel() = default;

WireGuardTunnel::~WireGuardTunnel() {
    stop();
}

bool WireGuardTunnel::start(const WireGuardConfig& config, const std::string& conf_path) {
    if (is_running_) return true;

    active_endpoint_ = config.endpoint_host;

    // Get executable directory to ensure reliable relative resolution
    wchar_t modPath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, modPath, MAX_PATH);
    wchar_t* pSlash = wcsrchr(modPath, L'\\');
    std::wstring appDir = pSlash ? std::wstring(modPath, pSlash + 1) : L"";

    std::string actual_conf = conf_path;

    // Create temporary .conf file if no explicit path was provided
    if (actual_conf.empty()) {
        std::wstring tempConf = appDir + L"active_wg.conf";
        int len = WideCharToMultiByte(CP_UTF8, 0, tempConf.c_str(), -1, NULL, 0, NULL, NULL);
        if (len > 0) {
            std::string tempStr(len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, tempConf.c_str(), -1, &tempStr[0], len, NULL, NULL);
            actual_conf = tempStr;
        } else {
            actual_conf = "active_wg.conf";
        }

        std::ofstream ofs(actual_conf);
        if (!ofs.is_open()) {
            ProxyController::instance().log("[WireGuard] Failed to write temporary config file.");
            return false;
        }

        ofs << "[Interface]\n"
            << "PrivateKey = " << config.private_key << "\n"
            << "Address = " << config.interface_address << "\n"
            << "DNS = " << config.dns << "\n\n"
            << "[Peer]\n"
            << "PublicKey = " << config.public_key << "\n"
            << "Endpoint = " << config.endpoint_host << ":" << config.endpoint_port << "\n"
            << "AllowedIPs = " << config.allowed_ips << "\n"
            << "PersistentKeepalive = " << config.persistent_keepalive << "\n";
        ofs.close();
    }

    // Candidate search paths for fissio-wg.exe
    std::vector<std::wstring> candidates;
    if (!appDir.empty()) {
        candidates.push_back(appDir + L"fissio-wg.exe");
        candidates.push_back(appDir + L"build\\fissio-wg.exe");
        candidates.push_back(appDir + L"..\\build\\fissio-wg.exe");
    }
    candidates.push_back(L"fissio-wg.exe");
    candidates.push_back(L"build\\fissio-wg.exe");

    std::wstring found_wg_exe;
    std::wstring found_working_dir;

    for (const auto& path : candidates) {
        DWORD attr = GetFileAttributesW(path.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            wchar_t fullPath[MAX_PATH] = {0};
            if (GetFullPathNameW(path.c_str(), MAX_PATH, fullPath, NULL) > 0) {
                found_wg_exe = fullPath;
                wchar_t* p = wcsrchr(fullPath, L'\\');
                if (p) {
                    found_working_dir = std::wstring(fullPath, p);
                }
            } else {
                found_wg_exe = path;
            }
            break;
        }
    }

    if (found_wg_exe.empty()) {
        ProxyController::instance().log("[WireGuard] Error: fissio-wg.exe not found in application path.");
        return false;
    }

    // Create stdout/stderr pipe
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = NULL;
    HANDLE hWrite = NULL;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        ProxyController::instance().log("[WireGuard] Failed to create communication pipe.");
        return false;
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};

    // Convert actual_conf to UTF-16
    int confLen = MultiByteToWideChar(CP_UTF8, 0, actual_conf.c_str(), -1, NULL, 0);
    std::wstring conf_w(confLen > 0 ? confLen - 1 : 0, L'\0');
    if (confLen > 0) {
        MultiByteToWideChar(CP_UTF8, 0, actual_conf.c_str(), -1, &conf_w[0], confLen);
    }

    std::wstring cmd = L"\"" + found_wg_exe + L"\" -config \"" + conf_w + L"\"";
    const wchar_t* pWorkDir = found_working_dir.empty() ? NULL : found_working_dir.c_str();

    BOOL ok = CreateProcessW(NULL, &cmd[0], NULL, NULL, TRUE,
                             CREATE_NO_WINDOW, NULL, pWorkDir, &si, &pi);

    CloseHandle(hWrite); // Close parent write end of the pipe

    if (!ok) {
        CloseHandle(hRead);
        DWORD err = GetLastError();
        ProxyController::instance().log("[WireGuard] Failed to launch WireGuard Engine (ErrorCode: " + std::to_string(err) + ")");
        return false;
    }

    process_handle_ = pi.hProcess;
    thread_handle_ = pi.hThread;
    pipe_read_ = hRead;
    is_running_ = true;

    int cmdLen = WideCharToMultiByte(CP_UTF8, 0, cmd.c_str(), -1, NULL, 0, NULL, NULL);
    std::string cmdStr(cmdLen > 0 ? cmdLen - 1 : 0, '\0');
    if (cmdLen > 0) {
        WideCharToMultiByte(CP_UTF8, 0, cmd.c_str(), -1, &cmdStr[0], cmdLen, NULL, NULL);
    }
    ProxyController::instance().log("[WireGuard] Launching WireGuard engine: " + cmdStr);

    // Monitoring thread for child stdout/stderr pipe
    log_reader_thread_ = std::make_unique<std::thread>([this, hRead]() {
        char buf[512];
        DWORD bytesRead = 0;
        std::string line_acc;

        while (is_running_) {
            BOOL success = ReadFile(hRead, buf, sizeof(buf) - 1, &bytesRead, NULL);
            if (!success || bytesRead == 0) break;

            buf[bytesRead] = '\0';
            line_acc += buf;

            size_t newline_pos;
            while ((newline_pos = line_acc.find('\n')) != std::string::npos) {
                std::string line = line_acc.substr(0, newline_pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                line_acc = line_acc.substr(newline_pos + 1);

                if (!line.empty()) {
                    ProxyController::instance().log(line);
                }
            }
        }
        is_running_ = false;
    });

    return true;
}

void WireGuardTunnel::stop() {
    if (!is_running_ && !process_handle_) return;

    is_running_ = false;

    if (process_handle_) {
        // Terminate engine process
        TerminateProcess((HANDLE)process_handle_, 0);
        WaitForSingleObject((HANDLE)process_handle_, 2000);
        CloseHandle((HANDLE)process_handle_);
        process_handle_ = nullptr;
    }

    if (thread_handle_) {
        CloseHandle((HANDLE)thread_handle_);
        thread_handle_ = nullptr;
    }

    if (pipe_read_) {
        CloseHandle((HANDLE)pipe_read_);
        pipe_read_ = nullptr;
    }

    if (log_reader_thread_ && log_reader_thread_->joinable()) {
        log_reader_thread_->join();
    }
    log_reader_thread_.reset();

    // Clean up Wintun adapter routing rules
    system("route delete 0.0.0.0 mask 128.0.0.0 >nul 2>&1");
    system("route delete 128.0.0.0 mask 128.0.0.0 >nul 2>&1");
    if (!active_endpoint_.empty()) {
        std::string cmd = "route delete " + active_endpoint_ + " >nul 2>&1";
        system(cmd.c_str());
        active_endpoint_.clear();
    }

    ProxyController::instance().log("[WireGuard] WireGuard Tunnel safely disconnected.");
}

} // namespace fissio
#else
namespace fissio {
WireGuardTunnel& WireGuardTunnel::instance() { static WireGuardTunnel inst; return inst; }
WireGuardTunnel::WireGuardTunnel() = default;
WireGuardTunnel::~WireGuardTunnel() = default;
bool WireGuardTunnel::start(const WireGuardConfig&, const std::string&) { return false; }
void WireGuardTunnel::stop() {}
}
#endif

#include "wireguard_tunnel.hpp"
#include "proxy_controller.hpp"
#include <fstream>
#include <iostream>

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

    std::string actual_conf = conf_path;

    // Create temporary .conf file if no explicit path was provided
    if (actual_conf.empty()) {
        actual_conf = "active_wg.conf";
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

    // Verify fissio-wg.exe binary location
    std::string exe_path = "fissio-wg.exe";
    DWORD attr = GetFileAttributesA(exe_path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        exe_path = "build\\fissio-wg.exe";
        attr = GetFileAttributesA(exe_path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            ProxyController::instance().log("[WireGuard] Error: fissio-wg.exe not found.");
            return false;
        }
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

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    std::string cmd = exe_path + " -config \"" + actual_conf + "\"";

    BOOL ok = CreateProcessA(NULL, &cmd[0], NULL, NULL, TRUE,
                             CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

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

    ProxyController::instance().log("[WireGuard] Launching official WireGuard engine: " + cmd);

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

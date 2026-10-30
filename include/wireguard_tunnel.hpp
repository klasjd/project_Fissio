#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <memory>
#include "wireguard_config.hpp"

namespace fissio {

class WireGuardTunnel {
public:
    static WireGuardTunnel& instance();

    bool start(const WireGuardConfig& config, const std::string& conf_path = "");
    void stop();
    bool is_running() const { return is_running_; }

private:
    WireGuardTunnel();
    ~WireGuardTunnel();

    std::atomic<bool> is_running_{false};
    std::unique_ptr<std::thread> log_reader_thread_;
    std::string active_endpoint_;

    void* process_handle_ = nullptr;
    void* thread_handle_ = nullptr;
    void* pipe_read_ = nullptr;
};

} // namespace fissio

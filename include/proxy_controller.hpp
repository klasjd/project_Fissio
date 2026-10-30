#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>
#include <asio.hpp>
#include "tls_fragment.hpp"
#include "proxy_session.hpp"

namespace fissio {

using LogCallback = std::function<void(const std::string&)>;

class ProxyController {
public:
    static ProxyController& instance();

    bool start(uint16_t port, FragmentConfig frag_cfg, bool auto_system_proxy);
    void stop();
    bool is_running() const { return is_running_; }

    void set_log_callback(LogCallback cb);
    void log(const std::string& msg);

private:
    ProxyController();
    ~ProxyController();

    std::atomic<bool> is_running_{false};
    bool auto_system_proxy_{false};
    uint16_t current_port_{10808};

    std::unique_ptr<std::thread> worker_thread_;
    std::unique_ptr<asio::io_context> ioc_;
    std::unique_ptr<ProxyServer> server_;

    LogCallback log_cb_;
    std::mutex log_mutex_;
};

} // namespace fissio

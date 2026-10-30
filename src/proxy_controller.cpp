#include "proxy_controller.hpp"
#include "system_proxy.hpp"
#include <iostream>

namespace fissio {

ProxyController& ProxyController::instance() {
    static ProxyController inst;
    return inst;
}

ProxyController::ProxyController() = default;

ProxyController::~ProxyController() {
    stop();
}

void ProxyController::set_log_callback(LogCallback cb) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    log_cb_ = std::move(cb);
}

void ProxyController::log(const std::string& msg) {
    std::cout << msg << std::endl;
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (log_cb_) {
        log_cb_(msg);
    }
}

bool ProxyController::start(uint16_t port, FragmentConfig frag_cfg, bool auto_system_proxy) {
    if (is_running_) return true;

    current_port_ = port;
    auto_system_proxy_ = auto_system_proxy;

    try {
        ioc_ = std::make_unique<asio::io_context>();
        server_ = std::make_unique<ProxyServer>(*ioc_, port, frag_cfg);
        server_->start();

        is_running_ = true;

        worker_thread_ = std::make_unique<std::thread>([this]() {
            try {
                ioc_->run();
            } catch (const std::exception& e) {
                log("[Controller] Asio run exception: " + std::string(e.what()));
            }
            is_running_ = false;
        });

        if (auto_system_proxy_) {
            SystemProxy::enable_socks5(current_port_);
            log("[Controller] Windows system SOCKS5 proxy enabled (127.0.0.1:" + std::to_string(current_port_) + ")");
        }

        log("[Controller] Fissio Engine Started on 127.0.0.1:" + std::to_string(current_port_));
        return true;
    } catch (const std::exception& e) {
        log("[Controller] Failed to start engine: " + std::string(e.what()));
        stop();
        return false;
    }
}

void ProxyController::stop() {
    if (!is_running_) return;

    if (auto_system_proxy_) {
        SystemProxy::disable();
        log("[Controller] Windows system proxy disabled.");
    }

    if (ioc_) {
        ioc_->stop();
    }

    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }

    server_.reset();
    ioc_.reset();
    worker_thread_.reset();

    is_running_ = false;
    log("[Controller] Fissio Engine Stopped.");
}

} // namespace fissio

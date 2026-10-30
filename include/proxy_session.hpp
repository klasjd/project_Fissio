#pragma once

#include <memory>
#include <vector>
#include <string>
#include <asio.hpp>
#include "socks5.hpp"
#include "doh.hpp"
#include "tls_fragment.hpp"

namespace fissio {

class ProxySession : public std::enable_shared_from_this<ProxySession> {
public:
    ProxySession(asio::ip::tcp::socket client_sock,
                 DohClient& doh_client,
                 SSL_CTX* ssl_ctx,
                 FragmentConfig frag_cfg);

    void start();

private:
    asio::awaitable<void> run();
    asio::awaitable<void> relay_streams(asio::ip::tcp::socket& local, asio::ip::tcp::socket& remote);

    asio::ip::tcp::socket client_sock_;
    DohClient& doh_client_;
    SSL_CTX* ssl_ctx_;
    FragmentConfig frag_cfg_;
};

class ProxyServer {
public:
    ProxyServer(asio::io_context& ioc, uint16_t port, FragmentConfig frag_cfg);
    ~ProxyServer();

    void start();

private:
    asio::awaitable<void> accept_loop();

    asio::io_context& ioc_;
    uint16_t port_;
    FragmentConfig frag_cfg_;
    asio::ip::tcp::acceptor acceptor_;
    DohClient doh_client_;
    SSL_CTX* ssl_ctx_ = nullptr;
};

} // namespace fissio

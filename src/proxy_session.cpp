#include "proxy_session.hpp"
#include "proxy_controller.hpp"
#include <iostream>
#include <array>
#include <asio/steady_timer.hpp>
#include <asio/experimental/awaitable_operators.hpp>

using namespace asio::experimental::awaitable_operators;

namespace fissio {

ProxySession::ProxySession(asio::ip::tcp::socket client_sock,
                           DohClient& doh_client,
                           SSL_CTX* ssl_ctx,
                           FragmentConfig frag_cfg)
    : client_sock_(std::move(client_sock)),
      doh_client_(doh_client),
      ssl_ctx_(ssl_ctx),
      frag_cfg_(frag_cfg) {}

void ProxySession::start() {
    auto self = shared_from_this();
    asio::co_spawn(client_sock_.get_executor(), [self]() -> asio::awaitable<void> {
        co_await self->run();
    }, asio::detached);
}

asio::awaitable<void> ProxySession::run() {
    try {
        std::vector<uint8_t> buf(4096);
        size_t n = co_await client_sock_.async_read_some(asio::buffer(buf), asio::use_awaitable);
        if (n == 0) co_return;

        auto proto = Socks5Handler::detect_protocol(buf.data(), n);
        Socks5Request req;
        size_t consumed = 0;

        if (proto == ClientProtocol::Socks5) {
            // SOCKS5 negotiation (RFC 1928)
            if (!Socks5Handler::parse_auth_handshake(buf.data(), n, consumed)) {
                co_return;
            }
            auto auth_resp = Socks5Handler::make_auth_response(0x00);
            co_await asio::async_write(client_sock_, asio::buffer(auth_resp), asio::use_awaitable);

            n = co_await client_sock_.async_read_some(asio::buffer(buf), asio::use_awaitable);
            auto req_opt = Socks5Handler::parse_request(buf.data(), n, consumed);
            if (!req_opt.has_value() || req_opt->command != SocksCommand::Connect) {
                auto err_resp = Socks5Handler::make_reply(SocksReply::GeneralFailure);
                co_await asio::async_write(client_sock_, asio::buffer(err_resp), asio::use_awaitable);
                co_return;
            }
            req = req_opt.value();
        } else if (proto == ClientProtocol::Socks4) {
            // SOCKS4 / SOCKS4a (Pre-negotiated in first packet)
            auto req_opt = Socks5Handler::parse_socks4_request(buf.data(), n, consumed);
            if (!req_opt.has_value()) {
                auto err_resp = Socks5Handler::make_socks4_reply(false);
                co_await asio::async_write(client_sock_, asio::buffer(err_resp), asio::use_awaitable);
                co_return;
            }
            req = req_opt.value();
        } else if (proto == ClientProtocol::HttpConnect) {
            // HTTP CONNECT (RFC 7231)
            auto req_opt = Socks5Handler::parse_http_connect_request(buf.data(), n, consumed);
            if (!req_opt.has_value()) {
                std::string err_resp = Socks5Handler::make_http_connect_reply(false);
                co_await asio::async_write(client_sock_, asio::buffer(err_resp), asio::use_awaitable);
                co_return;
            }
            req = req_opt.value();
        } else {
            co_return;
        }

        // Resolve destination host via DoH (or fallback to system DNS)
        std::string target_ip = req.target_host;
        if (req.is_domain) {
            try {
                auto doh_res = co_await doh_client_.resolve(req.target_host);
                if (doh_res.has_value() && !doh_res->ip.empty()) {
                    target_ip = doh_res->ip;
                }
            } catch (...) {
                // Fallback to standard DNS resolver
            }
        }

        // Establish TCP connection to remote endpoint
        auto executor = client_sock_.get_executor();
        asio::ip::tcp::resolver resolver(executor);
        auto endpoints = co_await resolver.async_resolve(target_ip, std::to_string(req.target_port), asio::use_awaitable);

        asio::ip::tcp::socket remote_sock(executor);
        co_await asio::async_connect(remote_sock, endpoints, asio::use_awaitable);

        // Send protocol-appropriate success response to client
        if (proto == ClientProtocol::Socks5) {
            auto ok_resp = Socks5Handler::make_reply(SocksReply::Success);
            co_await asio::async_write(client_sock_, asio::buffer(ok_resp), asio::use_awaitable);
        } else if (proto == ClientProtocol::Socks4) {
            auto ok_resp = Socks5Handler::make_socks4_reply(true);
            co_await asio::async_write(client_sock_, asio::buffer(ok_resp), asio::use_awaitable);
        } else if (proto == ClientProtocol::HttpConnect) {
            std::string ok_resp = Socks5Handler::make_http_connect_reply(true);
            co_await asio::async_write(client_sock_, asio::buffer(ok_resp), asio::use_awaitable);
        }

        ProxyController::instance().log("[Fissio] Tunnel established: " + req.target_host + ":" + std::to_string(req.target_port) + " via " + target_ip);

        // Bidirectional non-blocking streaming with asynchronous TCP fragmentation
        co_await relay_streams(client_sock_, remote_sock);

    } catch (const std::exception&) {
        // Socket closed or connection aborted
    }
}

asio::awaitable<void> ProxySession::relay_streams(asio::ip::tcp::socket& local, asio::ip::tcp::socket& remote) {
    // Forward from local client to remote target
    auto local_to_remote = [&]() -> asio::awaitable<void> {
        std::array<uint8_t, 16384> buf;
        bool is_first_packet = true;

        while (true) {
            size_t bytes = co_await local.async_read_some(asio::buffer(buf), asio::use_awaitable);
            if (bytes == 0) break;

            if (is_first_packet && frag_cfg_.enable_fragment) {
                // Detect TLS Handshake ClientHello (ContentType=0x16, Version=0x03)
                if (bytes >= 5 && buf[0] == 0x16 && buf[1] == 0x03) {
                    size_t split = (bytes > frag_cfg_.split_size) ? frag_cfg_.split_size : bytes;

                    // Asynchronously transmit first slice
                    co_await asio::async_write(remote, asio::buffer(buf.data(), split), asio::use_awaitable);

                    if (split < bytes) {
                        if (frag_cfg_.delay_ms > 0) {
                            asio::steady_timer timer(remote.get_executor());
                            timer.expires_after(std::chrono::milliseconds(frag_cfg_.delay_ms));
                            co_await timer.async_wait(asio::use_awaitable);
                        }
                        // Asynchronously transmit remaining slice
                        co_await asio::async_write(remote, asio::buffer(buf.data() + split, bytes - split), asio::use_awaitable);
                    }

                    is_first_packet = false;
                    continue;
                }
                is_first_packet = false;
            }

            co_await asio::async_write(remote, asio::buffer(buf.data(), bytes), asio::use_awaitable);
        }
        std::error_code ec;
        remote.shutdown(asio::ip::tcp::socket::shutdown_send, ec);
    };

    // Forward from remote target to local client
    auto remote_to_local = [&]() -> asio::awaitable<void> {
        std::array<uint8_t, 16384> buf;
        while (true) {
            size_t bytes = co_await remote.async_read_some(asio::buffer(buf), asio::use_awaitable);
            if (bytes == 0) break;
            co_await asio::async_write(local, asio::buffer(buf.data(), bytes), asio::use_awaitable);
        }
        std::error_code ec;
        local.shutdown(asio::ip::tcp::socket::shutdown_send, ec);
    };

    // Run both directions concurrently
    try {
        co_await (local_to_remote() && remote_to_local());
    } catch (...) {
        // Socket shutdown
    }
}

ProxyServer::ProxyServer(asio::io_context& ioc, uint16_t port, FragmentConfig frag_cfg)
    : ioc_(ioc),
      port_(port),
      frag_cfg_(frag_cfg),
      acceptor_(ioc, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      doh_client_(ioc) {
    ssl_ctx_ = TlsSessionManager::create_client_context();
}

ProxyServer::~ProxyServer() {
    if (ssl_ctx_) {
        TlsSessionManager::destroy_context(ssl_ctx_);
    }
}

void ProxyServer::start() {
    std::cout << "[Fissio] Server started on 127.0.0.1:" << port_ << " (SOCKS5/SOCKS4/HTTP)\n";
    std::cout << "[Fissio] Fragmentation config: Split=" << frag_cfg_.split_size 
              << " bytes, Delay=" << frag_cfg_.delay_ms << " ms\n";
    std::cout << "[Fissio] Ready to accept connections from browser/apps...\n";

    asio::co_spawn(ioc_, accept_loop(), asio::detached);
}

asio::awaitable<void> ProxyServer::accept_loop() {
    while (true) {
        auto socket = co_await acceptor_.async_accept(asio::use_awaitable);
        auto session = std::make_shared<ProxySession>(std::move(socket), doh_client_, ssl_ctx_, frag_cfg_);
        session->start();
    }
}

} // namespace fissio

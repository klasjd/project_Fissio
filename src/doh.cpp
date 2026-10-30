#include "doh.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace fissio {

DohClient::DohClient(asio::io_context& ioc, std::string doh_server_ip, uint16_t doh_port)
    : ioc_(ioc), doh_ip_(std::move(doh_server_ip)), doh_port_(doh_port) {}

std::optional<DohResult> DohClient::get_from_cache(const std::string& domain) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = cache_.find(domain);
    if (it != cache_.end()) {
        if (std::chrono::steady_clock::now() < it->second.expires_at) {
            return it->second;
        }
        cache_.erase(it);
    }
    return std::nullopt;
}

void DohClient::save_to_cache(const std::string& domain, const DohResult& res) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_[domain] = res;
}

std::vector<uint8_t> DohClient::build_wire_query(const std::string& domain, uint16_t qtype, uint16_t tx_id) {
    std::vector<uint8_t> packet;
    packet.reserve(12 + domain.size() + 2 + 4);

    // DNS Header (12 bytes)
    packet.push_back(static_cast<uint8_t>((tx_id >> 8) & 0xFF));
    packet.push_back(static_cast<uint8_t>(tx_id & 0xFF));
    packet.push_back(0x01); // Flags: RD = 1
    packet.push_back(0x00);
    packet.push_back(0x00); packet.push_back(0x01); // QDCOUNT = 1
    packet.push_back(0x00); packet.push_back(0x00); // ANCOUNT = 0
    packet.push_back(0x00); packet.push_back(0x00); // NSCOUNT = 0
    packet.push_back(0x00); packet.push_back(0x00); // ARCOUNT = 0

    // QNAME (labels: www.example.com -> 3www7example3com0)
    std::stringstream ss(domain);
    std::string label;
    while (std::getline(ss, label, '.')) {
        if (!label.empty()) {
            packet.push_back(static_cast<uint8_t>(label.size()));
            packet.insert(packet.end(), label.begin(), label.end());
        }
    }
    packet.push_back(0x00); // Root label null terminator

    // QTYPE
    packet.push_back(static_cast<uint8_t>((qtype >> 8) & 0xFF));
    packet.push_back(static_cast<uint8_t>(qtype & 0xFF));

    // QCLASS = 1 (IN)
    packet.push_back(0x00);
    packet.push_back(0x01);

    return packet;
}

// Helper: Skip DNS domain name (including wire compression pointers)
static bool skip_dns_name(const uint8_t* data, size_t len, size_t& offset) {
    while (offset < len) {
        uint8_t label_len = data[offset];
        if (label_len == 0) {
            offset += 1;
            return true;
        }
        if ((label_len & 0xC0) == 0xC0) {
            // Compressed pointer (2 bytes)
            offset += 2;
            return true;
        }
        offset += 1 + label_len;
    }
    return false;
}

bool DohClient::parse_wire_response(const uint8_t* data, size_t len,
                                    std::vector<std::string>& out_ips,
                                    std::vector<uint8_t>& out_ech_config) {
    if (len < 12) return false;

    // Header check
    uint16_t flags = (data[2] << 8) | data[3];
    uint8_t rcode = flags & 0x0F;
    if (rcode != 0) return false; // DNS error

    uint16_t qdcount = (data[4] << 8) | data[5];
    uint16_t ancount = (data[6] << 8) | data[7];

    size_t offset = 12;

    // Skip Questions
    for (uint16_t i = 0; i < qdcount; ++i) {
        if (!skip_dns_name(data, len, offset)) return false;
        offset += 4; // QTYPE (2) + QCLASS (2)
        if (offset > len) return false;
    }

    // Parse Answers
    for (uint16_t i = 0; i < ancount; ++i) {
        if (!skip_dns_name(data, len, offset)) return false;
        if (offset + 10 > len) return false;

        uint16_t type = (data[offset] << 8) | data[offset + 1];
        // uint16_t cls = (data[offset + 2] << 8) | data[offset + 3];
        // uint32_t ttl = (data[offset + 4] << 24) | ...
        uint16_t rdlen = (data[offset + 8] << 8) | data[offset + 9];
        offset += 10;

        if (offset + rdlen > len) return false;

        if (type == 1 && rdlen == 4) { // A (IPv4)
            char ip_buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, data + offset, ip_buf, sizeof(ip_buf));
            out_ips.emplace_back(ip_buf);
        } else if (type == 65) { // HTTPS RR (RFC 9460)
            // RDATA format:
            // SvcPriority: 2 bytes
            // TargetName: wire format
            // SvcParams: [Key: 2B][Len: 2B][Value]
            if (rdlen >= 3) {
                size_t r_offset = offset;
                size_t r_end = offset + rdlen;

                // uint16_t priority = (data[r_offset] << 8) | data[r_offset + 1];
                r_offset += 2;

                if (!skip_dns_name(data, len, r_offset)) {
                    offset += rdlen;
                    continue;
                }

                while (r_offset + 4 <= r_end) {
                    uint16_t key = (data[r_offset] << 8) | data[r_offset + 1];
                    uint16_t key_len = (data[r_offset + 2] << 8) | data[r_offset + 3];
                    r_offset += 4;

                    if (r_offset + key_len > r_end) break;

                    if (key == 0x0005) { // SvcParamKey 'ech'
                        out_ech_config.assign(data + r_offset, data + r_offset + key_len);
                    }
                    r_offset += key_len;
                }
            }
        }
        offset += rdlen;
    }

    return true;
}

// Send HTTPS DoH query directly to 1.1.1.1:443 (Asio coroutine based)
asio::awaitable<std::optional<DohResult>> DohClient::resolve(std::string domain) {
    if (auto cached = get_from_cache(domain)) {
        co_return cached;
    }

    DohResult result;
    result.expires_at = std::chrono::steady_clock::now() + std::chrono::seconds(300);

    // 1. Build Type 65 (HTTPS) Wire Query
    auto https_query = build_wire_query(domain, 65, 0x1111);
    // 2. Build Type 1 (A) Wire Query
    auto a_query = build_wire_query(domain, 1, 0x2222);

    try {
        asio::ip::tcp::resolver resolver(ioc_);
        auto endpoints = co_await resolver.async_resolve(doh_ip_, std::to_string(doh_port_), asio::use_awaitable);

        // Raw TCP connection
        asio::ip::tcp::socket raw_sock(ioc_);
        co_await asio::async_connect(raw_sock, endpoints, asio::use_awaitable);

        // Initialize DoH SSL_CTX
        SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
        SSL* ssl = SSL_new(ctx);
        SSL_set_tlsext_host_name(ssl, "cloudflare-dns.com");

        BIO* bio = BIO_new_socket(static_cast<int>(raw_sock.native_handle()), BIO_NOCLOSE);
        SSL_set_bio(ssl, bio, bio);

        if (SSL_connect(ssl) <= 0) {
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            co_return std::nullopt;
        }

        // Send HTTPS RR query
        std::string req1 = "POST /dns-query HTTP/1.1\r\n"
                           "Host: 1.1.1.1\r\n"
                           "Content-Type: application/dns-message\r\n"
                           "Accept: application/dns-message\r\n"
                           "Content-Length: " + std::to_string(https_query.size()) + "\r\n"
                           "Connection: keep-alive\r\n\r\n";
        SSL_write(ssl, req1.data(), static_cast<int>(req1.size()));
        SSL_write(ssl, https_query.data(), static_cast<int>(https_query.size()));

        // Response buffer
        std::vector<uint8_t> resp_buf(8192);
        int bytes = SSL_read(ssl, resp_buf.data(), static_cast<int>(resp_buf.size()));
        if (bytes > 0) {
            std::string resp_str(reinterpret_cast<char*>(resp_buf.data()), bytes);
            size_t body_pos = resp_str.find("\r\n\r\n");
            if (body_pos != std::string::npos) {
                const uint8_t* wire_data = resp_buf.data() + body_pos + 4;
                size_t wire_len = bytes - (body_pos + 4);
                std::vector<std::string> ips;
                std::vector<uint8_t> ech_config;
                if (parse_wire_response(wire_data, wire_len, ips, ech_config)) {
                    if (!ech_config.empty()) {
                        result.ech_config_list = std::move(ech_config);
                        result.has_ech = true;
                    }
                    if (!ips.empty()) {
                        result.ip = ips.front();
                    }
                }
            }
        }

        // If IP is not found yet, send A record query
        if (result.ip.empty()) {
            std::string req2 = "POST /dns-query HTTP/1.1\r\n"
                               "Host: 1.1.1.1\r\n"
                               "Content-Type: application/dns-message\r\n"
                               "Accept: application/dns-message\r\n"
                               "Content-Length: " + std::to_string(a_query.size()) + "\r\n"
                               "Connection: close\r\n\r\n";
            SSL_write(ssl, req2.data(), static_cast<int>(req2.size()));
            SSL_write(ssl, a_query.data(), static_cast<int>(a_query.size()));

            bytes = SSL_read(ssl, resp_buf.data(), static_cast<int>(resp_buf.size()));
            if (bytes > 0) {
                std::string resp_str(reinterpret_cast<char*>(resp_buf.data()), bytes);
                size_t body_pos = resp_str.find("\r\n\r\n");
                if (body_pos != std::string::npos) {
                    const uint8_t* wire_data = resp_buf.data() + body_pos + 4;
                    size_t wire_len = bytes - (body_pos + 4);
                    std::vector<std::string> ips;
                    std::vector<uint8_t> ech_cfg;
                    if (parse_wire_response(wire_data, wire_len, ips, ech_cfg)) {
                        if (!ips.empty()) {
                            result.ip = ips.front();
                        }
                    }
                }
            }
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);

    } catch (const std::exception& e) {
        std::cerr << "[DoH] Error resolving " << domain << ": " << e.what() << "\n";
        co_return std::nullopt;
    }

    if (!result.ip.empty() || result.has_ech) {
        save_to_cache(domain, result);
        co_return result;
    }

    co_return std::nullopt;
}

} // namespace fissio

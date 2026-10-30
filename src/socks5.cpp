#include "socks5.hpp"
#include <cstring>
#include <string_view>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace fissio {

ClientProtocol Socks5Handler::detect_protocol(const uint8_t* data, size_t len) {
    if (len == 0) return ClientProtocol::Unknown;

    if (data[0] == 0x05) {
        return ClientProtocol::Socks5;
    }
    if (data[0] == 0x04) {
        return ClientProtocol::Socks4;
    }
    if (len >= 7 && std::memcmp(data, "CONNECT", 7) == 0) {
        return ClientProtocol::HttpConnect;
    }

    return ClientProtocol::Unknown;
}

// -------------------------------------------------------------
// SOCKS5 (RFC 1928) Implementation
// -------------------------------------------------------------
bool Socks5Handler::parse_auth_handshake(const uint8_t* data, size_t len, size_t& consumed) {
    if (len < 2) return false;
    uint8_t ver = data[0];
    if (ver != 0x05) return false;

    uint8_t nmethods = data[1];
    if (len < static_cast<size_t>(2 + nmethods)) return false;

    // Check if NO_AUTHENTICATION_REQUIRED (0x00) is supported
    bool has_no_auth = false;
    for (size_t i = 0; i < nmethods; ++i) {
        if (data[2 + i] == 0x00) {
            has_no_auth = true;
            break;
        }
    }

    if (!has_no_auth) return false;

    consumed = 2 + nmethods;
    return true;
}

std::vector<uint8_t> Socks5Handler::make_auth_response(uint8_t method) {
    return {0x05, method};
}

std::optional<Socks5Request> Socks5Handler::parse_request(const uint8_t* data, size_t len, size_t& consumed) {
    // SOCKS5 request: [VER(1)][CMD(1)][RSV(1)][ATYP(1)][DST.ADDR(...)][DST.PORT(2)]
    if (len < 4) return std::nullopt;
    if (data[0] != 0x05) return std::nullopt;
    if (data[2] != 0x00) return std::nullopt;

    Socks5Request req;
    req.command = static_cast<SocksCommand>(data[1]);
    req.address_type = static_cast<SocksAddressType>(data[3]);

    size_t offset = 4;
    if (req.address_type == SocksAddressType::IPv4) {
        if (len < offset + 4 + 2) return std::nullopt;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, data + offset, ip_str, sizeof(ip_str));
        req.target_host = ip_str;
        req.is_domain = false;
        offset += 4;
    } else if (req.address_type == SocksAddressType::DomainName) {
        if (len < offset + 1) return std::nullopt;
        uint8_t domain_len = data[offset++];
        if (len < offset + domain_len + 2) return std::nullopt;
        req.target_host.assign(reinterpret_cast<const char*>(data + offset), domain_len);
        req.is_domain = true;
        offset += domain_len;
    } else if (req.address_type == SocksAddressType::IPv6) {
        if (len < offset + 16 + 2) return std::nullopt;
        char ip_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, data + offset, ip_str, sizeof(ip_str));
        req.target_host = ip_str;
        req.is_domain = false;
        offset += 16;
    } else {
        return std::nullopt;
    }

    uint16_t port_net = 0;
    std::memcpy(&port_net, data + offset, 2);
    req.target_port = ntohs(port_net);
    offset += 2;

    consumed = offset;
    return req;
}

std::vector<uint8_t> Socks5Handler::make_reply(SocksReply reply, const std::string& bind_addr, uint16_t bind_port) {
    std::vector<uint8_t> resp;
    resp.reserve(10);
    resp.push_back(0x05); // VER
    resp.push_back(static_cast<uint8_t>(reply)); // REP
    resp.push_back(0x00); // RSV
    resp.push_back(0x01); // ATYP = IPv4

    in_addr addr4{};
    if (inet_pton(AF_INET, bind_addr.c_str(), &addr4) == 1) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&addr4);
        resp.insert(resp.end(), p, p + 4);
    } else {
        resp.insert(resp.end(), {0, 0, 0, 0});
    }

    uint16_t port_net = htons(bind_port);
    const uint8_t* port_p = reinterpret_cast<const uint8_t*>(&port_net);
    resp.push_back(port_p[0]);
    resp.push_back(port_p[1]);

    return resp;
}

// -------------------------------------------------------------
// SOCKS4 & SOCKS4a Implementation
// -------------------------------------------------------------
std::optional<Socks5Request> Socks5Handler::parse_socks4_request(const uint8_t* data, size_t len, size_t& consumed) {
    // SOCKS4 header: [VN(1)=0x04][CD(1)=0x01][DSTPORT(2)][DSTIP(4)][USERID(var)... 0x00]
    if (len < 8) return std::nullopt;
    if (data[0] != 0x04) return std::nullopt;
    if (data[1] != 0x01) return std::nullopt; // Only CONNECT (0x01) is supported

    uint16_t port = (static_cast<uint16_t>(data[2]) << 8) | static_cast<uint16_t>(data[3]);

    // Locate null terminator for USERID
    size_t user_end = 8;
    while (user_end < len && data[user_end] != 0x00) {
        user_end++;
    }
    if (user_end >= len) return std::nullopt; // Incomplete packet

    bool is_socks4a = (data[4] == 0 && data[5] == 0 && data[6] == 0 && data[7] != 0);

    Socks5Request req;
    req.command = SocksCommand::Connect;
    req.target_port = port;

    if (is_socks4a) {
        // SOCKS4a: Domain string follows USERID null terminator
        size_t dom_start = user_end + 1;
        size_t dom_end = dom_start;
        while (dom_end < len && data[dom_end] != 0x00) {
            dom_end++;
        }
        if (dom_end >= len) return std::nullopt;

        req.target_host.assign(reinterpret_cast<const char*>(data + dom_start), dom_end - dom_start);
        req.is_domain = true;
        req.address_type = SocksAddressType::DomainName;
        consumed = dom_end + 1;
    } else {
        // Standard SOCKS4: IPv4 in header
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, data + 4, ip_str, sizeof(ip_str));
        req.target_host = ip_str;
        req.is_domain = false;
        req.address_type = SocksAddressType::IPv4;
        consumed = user_end + 1;
    }

    return req;
}

std::vector<uint8_t> Socks5Handler::make_socks4_reply(bool granted) {
    // 8 bytes: [VN:0x00][CD:0x5A(granted)/0x5B(rejected)][DSTPORT:2B][DSTIP:4B]
    return {
        0x00,
        static_cast<uint8_t>(granted ? 0x5A : 0x5B),
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
}

// -------------------------------------------------------------
// HTTP CONNECT Proxy Implementation (RFC 7231)
// -------------------------------------------------------------
std::optional<Socks5Request> Socks5Handler::parse_http_connect_request(const uint8_t* data, size_t len, size_t& consumed) {
    std::string_view req_str(reinterpret_cast<const char*>(data), len);
    size_t header_end = req_str.find("\r\n\r\n");
    if (header_end == std::string_view::npos) {
        header_end = req_str.find("\n\n");
        if (header_end == std::string_view::npos) return std::nullopt;
        consumed = header_end + 2;
    } else {
        consumed = header_end + 4;
    }

    if (req_str.rfind("CONNECT ", 0) == 0) {
        size_t sp1 = 8;
        size_t sp2 = req_str.find(' ', sp1);
        if (sp2 == std::string_view::npos) return std::nullopt;

        std::string target = std::string(req_str.substr(sp1, sp2 - sp1));
        size_t colon = target.find(':');
        Socks5Request req;
        req.command = SocksCommand::Connect;

        if (colon != std::string::npos) {
            req.target_host = target.substr(0, colon);
            try {
                req.target_port = static_cast<uint16_t>(std::stoi(target.substr(colon + 1)));
            } catch (...) {
                req.target_port = 443;
            }
        } else {
            req.target_host = target;
            req.target_port = 443;
        }

        in_addr addr4{};
        if (inet_pton(AF_INET, req.target_host.c_str(), &addr4) == 1) {
            req.is_domain = false;
            req.address_type = SocksAddressType::IPv4;
        } else {
            req.is_domain = true;
            req.address_type = SocksAddressType::DomainName;
        }

        return req;
    }

    return std::nullopt;
}

std::string Socks5Handler::make_http_connect_reply(bool success) {
    if (success) {
        return "HTTP/1.1 200 Connection Established\r\nProxy-Agent: Fissio/1.0\r\n\r\n";
    } else {
        return "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";
    }
}

} // namespace fissio

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace fissio {

enum class ClientProtocol {
    Socks5,
    Socks4,
    HttpConnect,
    Unknown
};

enum class SocksAddressType : uint8_t {
    IPv4 = 0x01,
    DomainName = 0x03,
    IPv6 = 0x04
};

enum class SocksCommand : uint8_t {
    Connect = 0x01,
    Bind = 0x02,
    UdpAssociate = 0x03
};

enum class SocksReply : uint8_t {
    Success = 0x00,
    GeneralFailure = 0x01,
    ConnectionNotAllowed = 0x02,
    NetworkUnreachable = 0x03,
    HostUnreachable = 0x04,
    ConnectionRefused = 0x05,
    TtlExpired = 0x06,
    CommandNotSupported = 0x07,
    AddressTypeNotSupported = 0x08
};

struct Socks5Request {
    SocksCommand command = SocksCommand::Connect;
    SocksAddressType address_type = SocksAddressType::IPv4;
    std::string target_host;
    uint16_t target_port = 0;
    bool is_domain = false;
};

class Socks5Handler {
public:
    // Protocol detection
    static ClientProtocol detect_protocol(const uint8_t* data, size_t len);

    // SOCKS5 (RFC 1928)
    static bool parse_auth_handshake(const uint8_t* data, size_t len, size_t& consumed);
    static std::vector<uint8_t> make_auth_response(uint8_t method = 0x00);
    static std::optional<Socks5Request> parse_request(const uint8_t* data, size_t len, size_t& consumed);
    static std::vector<uint8_t> make_reply(SocksReply reply, const std::string& bind_addr = "0.0.0.0", uint16_t bind_port = 0);

    // SOCKS4 & SOCKS4a (RFC 1928 predecessor)
    static std::optional<Socks5Request> parse_socks4_request(const uint8_t* data, size_t len, size_t& consumed);
    static std::vector<uint8_t> make_socks4_reply(bool granted);

    // HTTP CONNECT (RFC 7231 Section 4.3.6)
    static std::optional<Socks5Request> parse_http_connect_request(const uint8_t* data, size_t len, size_t& consumed);
    static std::string make_http_connect_reply(bool success);
};

} // namespace fissio

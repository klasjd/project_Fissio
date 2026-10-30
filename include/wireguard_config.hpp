#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace fissio {

struct WireGuardConfig {
    // [Interface]
    std::string private_key;
    std::string interface_address; // e.g. "10.0.0.2/24"
    std::string dns = "1.1.1.1";
    uint16_t listen_port = 0;

    // [Peer]
    std::string public_key;
    std::string endpoint_host;    // e.g. "vpn.example.com" or "198.51.100.1"
    uint16_t endpoint_port = 51820;
    std::string allowed_ips = "0.0.0.0/0";
    uint16_t persistent_keepalive = 25;

    bool is_valid() const {
        return !private_key.empty() && !public_key.empty() && !endpoint_host.empty();
    }
};

class WireGuardConfigParser {
public:
    static std::optional<WireGuardConfig> parse_string(const std::string& ini_content);
    static std::optional<WireGuardConfig> parse_file(const std::string& file_path);
};

} // namespace fissio

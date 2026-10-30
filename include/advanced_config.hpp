#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace fissio {

struct AdvancedSettings {
    // 1. Packet & DPI Fragmentation
    int delay_ms = 0;                  // Artificial delay in ms (0ms for instantaneous transmission)
    size_t split_size = 5;             // 1st stage packet split size (1~1500 bytes)
    bool enable_fragment = true;       // Enable TCP fragmentation
    bool tcp_nodelay = true;           // Force TCP_NODELAY (Disable Nagle algorithm)

    // 2. WireGuard / Wintun Virtual Network
    std::string adapter_name = "FissioWG";
    int mtu = 1360;                    // Recommended MTU (Prevents short buffer: 1360)
    std::string client_ip = "10.66.66.2";
    int prefix_len = 24;               // Subnet prefix (24 = 255.255.255.0)
    std::string virtual_gateway = "10.66.66.1";
    std::string server_endpoint = "";  // Dynamically loaded from user config (.conf)
    int server_port = 51820;
    std::string private_key = "";      // User private key (never hardcoded)
    std::string public_key = "";       // Peer public key (never hardcoded)
    std::string preshared_key = "";    // Optional preshared key
    int keepalive = 25;                // Keepalive interval in seconds
    std::string dns = "1.1.1.1, 1.0.0.1";
    bool auto_detect_nic = true;
    int physical_nic_index = 0;        // Auto-detected physical NIC ifIndex
    std::string physical_gateway = ""; // Auto-detected default gateway

    // 3. DoH & ECH (DNS over HTTPS)
    bool enable_doh = true;
    std::string doh_url = "https://cloudflare-dns.com/dns-query";
    bool enable_ech = true;

    // 4. SOCKS5 Proxy
    std::string bind_address = "127.0.0.1"; // "0.0.0.0" allows LAN sharing
    int socks_port = 10808;
    bool enable_sysproxy = true;
    std::string proxy_bypass = "<local>;*.kr;192.168.*";

    // Methods
    void load();
    void save();
    void reset_defaults();
    bool export_wireguard_conf(const std::string& path);
    bool import_wireguard_conf(const std::string& path);
};

// Singleton configuration manager
class AdvancedConfigManager {
public:
    static AdvancedConfigManager& instance();
    AdvancedSettings& settings() { return settings_; }
    const AdvancedSettings& settings() const { return settings_; }
    void load() { settings_.load(); }
    void save() { settings_.save(); }

private:
    AdvancedConfigManager();
    AdvancedSettings settings_;
};

} // namespace fissio

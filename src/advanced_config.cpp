#include "advanced_config.hpp"
#include "wireguard_config.hpp"
#include <fstream>
#include <sstream>
#include <windows.h>

namespace fissio {

AdvancedConfigManager& AdvancedConfigManager::instance() {
    static AdvancedConfigManager inst;
    return inst;
}

AdvancedConfigManager::AdvancedConfigManager() {
    load();
}

void AdvancedSettings::reset_defaults() {
    delay_ms = 0;
    split_size = 5;
    enable_fragment = true;
    tcp_nodelay = true;

    adapter_name = "FissioWG";
    mtu = 1360;
    client_ip = "10.66.66.2";
    prefix_len = 24;
    virtual_gateway = "10.66.66.1";
    server_endpoint = "";
    server_port = 51820;
    private_key = "";
    public_key = "";
    preshared_key = "";
    keepalive = 25;
    dns = "1.1.1.1, 1.0.0.1";
    auto_detect_nic = true;
    physical_nic_index = 0;
    physical_gateway = "";

    enable_doh = true;
    doh_url = "https://cloudflare-dns.com/dns-query";
    enable_ech = true;

    bind_address = "127.0.0.1";
    socks_port = 10808;
    enable_sysproxy = true;
    proxy_bypass = "<local>;*.kr;192.168.*";
}

static std::string get_ini_path() {
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char* p = strrchr(exePath, '\\');
    if (p) {
        *(p + 1) = '\0';
        return std::string(exePath) + "fissio_advanced.ini";
    }
    return "fissio_advanced.ini";
}

void AdvancedSettings::load() {
    std::string path = get_ini_path();
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        // Fallback: try importing existing config.conf
        import_wireguard_conf("config.conf");
        return;
    }

    std::string line;
    std::string section;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        // trim
        while (!key.empty() && isspace(key.front())) key.erase(key.begin());
        while (!key.empty() && isspace(key.back())) key.pop_back();
        while (!val.empty() && isspace(val.front())) val.erase(val.begin());
        while (!val.empty() && isspace(val.back())) val.pop_back();

        if (section == "DPI") {
            if (key == "delay_ms") delay_ms = std::stoi(val);
            else if (key == "split_size") split_size = std::stoul(val);
            else if (key == "enable_fragment") enable_fragment = (val == "1" || val == "true");
            else if (key == "tcp_nodelay") tcp_nodelay = (val == "1" || val == "true");
        } else if (section == "WireGuard") {
            if (key == "adapter_name") adapter_name = val;
            else if (key == "mtu") mtu = std::stoi(val);
            else if (key == "client_ip") client_ip = val;
            else if (key == "prefix_len") prefix_len = std::stoi(val);
            else if (key == "virtual_gateway") virtual_gateway = val;
            else if (key == "server_endpoint") server_endpoint = val;
            else if (key == "server_port") server_port = std::stoi(val);
            else if (key == "private_key") private_key = val;
            else if (key == "public_key") public_key = val;
            else if (key == "preshared_key") preshared_key = val;
            else if (key == "keepalive") keepalive = std::stoi(val);
            else if (key == "dns") dns = val;
            else if (key == "auto_detect_nic") auto_detect_nic = (val == "1" || val == "true");
            else if (key == "physical_nic_index") physical_nic_index = std::stoi(val);
            else if (key == "physical_gateway") physical_gateway = val;
        } else if (section == "DoH") {
            if (key == "enable_doh") enable_doh = (val == "1" || val == "true");
            else if (key == "doh_url") doh_url = val;
            else if (key == "enable_ech") enable_ech = (val == "1" || val == "true");
        } else if (section == "SOCKS5") {
            if (key == "bind_address") bind_address = val;
            else if (key == "socks_port") socks_port = std::stoi(val);
            else if (key == "enable_sysproxy") enable_sysproxy = (val == "1" || val == "true");
            else if (key == "proxy_bypass") proxy_bypass = val;
        }
    }
}

void AdvancedSettings::save() {
    std::string path = get_ini_path();
    std::ofstream ofs(path);
    if (!ofs.is_open()) return;

    ofs << "# Fissio Advanced Configuration File\n\n";

    ofs << "[DPI]\n"
        << "delay_ms = " << delay_ms << "\n"
        << "split_size = " << split_size << "\n"
        << "enable_fragment = " << (enable_fragment ? "1" : "0") << "\n"
        << "tcp_nodelay = " << (tcp_nodelay ? "1" : "0") << "\n\n";

    ofs << "[WireGuard]\n"
        << "adapter_name = " << adapter_name << "\n"
        << "mtu = " << mtu << "\n"
        << "client_ip = " << client_ip << "\n"
        << "prefix_len = " << prefix_len << "\n"
        << "virtual_gateway = " << virtual_gateway << "\n"
        << "server_endpoint = " << server_endpoint << "\n"
        << "server_port = " << server_port << "\n"
        << "private_key = " << private_key << "\n"
        << "public_key = " << public_key << "\n"
        << "preshared_key = " << preshared_key << "\n"
        << "keepalive = " << keepalive << "\n"
        << "dns = " << dns << "\n"
        << "auto_detect_nic = " << (auto_detect_nic ? "1" : "0") << "\n"
        << "physical_nic_index = " << physical_nic_index << "\n"
        << "physical_gateway = " << physical_gateway << "\n\n";

    ofs << "[DoH]\n"
        << "enable_doh = " << (enable_doh ? "1" : "0") << "\n"
        << "doh_url = " << doh_url << "\n"
        << "enable_ech = " << (enable_ech ? "1" : "0") << "\n\n";

    ofs << "[SOCKS5]\n"
        << "bind_address = " << bind_address << "\n"
        << "socks_port = " << socks_port << "\n"
        << "enable_sysproxy = " << (enable_sysproxy ? "1" : "0") << "\n"
        << "proxy_bypass = " << proxy_bypass << "\n";

    ofs.close();

    // Automatically synchronize and export to config.conf as well
    export_wireguard_conf("config.conf");
}

bool AdvancedSettings::export_wireguard_conf(const std::string& path) {
    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;

    ofs << "[Interface]\n"
        << "PrivateKey = " << private_key << "\n"
        << "Address = " << client_ip << "/" << prefix_len << "\n"
        << "DNS = " << dns << "\n"
        << "MTU = " << mtu << "\n\n"
        << "[Peer]\n"
        << "PublicKey = " << public_key << "\n";
    if (!preshared_key.empty()) {
        ofs << "PresharedKey = " << preshared_key << "\n";
    }
    ofs << "Endpoint = " << server_endpoint << ":" << server_port << "\n"
        << "AllowedIPs = 0.0.0.0/0, ::/0\n"
        << "PersistentKeepalive = " << keepalive << "\n";

    return true;
}

bool AdvancedSettings::import_wireguard_conf(const std::string& path) {
    auto parsed = WireGuardConfigParser::parse_file(path);
    if (!parsed.has_value()) return false;

    const auto& c = parsed.value();
    if (!c.private_key.empty()) private_key = c.private_key;
    if (!c.public_key.empty()) public_key = c.public_key;
    if (!c.endpoint_host.empty()) server_endpoint = c.endpoint_host;
    if (c.endpoint_port > 0) server_port = c.endpoint_port;
    if (c.persistent_keepalive > 0) keepalive = c.persistent_keepalive;
    if (!c.dns.empty()) dns = c.dns;
    if (!c.interface_address.empty()) {
        auto slash = c.interface_address.find('/');
        if (slash != std::string::npos) {
            client_ip = c.interface_address.substr(0, slash);
            prefix_len = std::stoi(c.interface_address.substr(slash + 1));
        } else {
            client_ip = c.interface_address;
        }
    }
    return true;
}

} // namespace fissio

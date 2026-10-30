#include "wireguard_config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace fissio {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::optional<WireGuardConfig> WireGuardConfigParser::parse_string(const std::string& ini_content) {
    WireGuardConfig cfg;
    std::istringstream stream(ini_content);
    std::string line;
    std::string current_section;

    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        if (line.front() == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            std::transform(current_section.begin(), current_section.end(), current_section.begin(), ::tolower);
            continue;
        }

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq_pos));
        std::string val = trim(line.substr(eq_pos + 1));
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        if (current_section == "interface") {
            if (key == "privatekey") {
                cfg.private_key = val;
            } else if (key == "address") {
                cfg.interface_address = val;
            } else if (key == "dns") {
                cfg.dns = val;
            } else if (key == "listenport") {
                cfg.listen_port = static_cast<uint16_t>(std::stoi(val));
            }
        } else if (current_section == "peer") {
            if (key == "publickey") {
                cfg.public_key = val;
            } else if (key == "endpoint") {
                size_t colon_pos = val.rfind(':');
                if (colon_pos != std::string::npos) {
                    cfg.endpoint_host = val.substr(0, colon_pos);
                    cfg.endpoint_port = static_cast<uint16_t>(std::stoi(val.substr(colon_pos + 1)));
                } else {
                    cfg.endpoint_host = val;
                }
            } else if (key == "allowedips") {
                cfg.allowed_ips = val;
            } else if (key == "persistentkeepalive") {
                cfg.persistent_keepalive = static_cast<uint16_t>(std::stoi(val));
            }
        }
    }

    if (cfg.is_valid()) {
        return cfg;
    }
    return std::nullopt;
}

std::optional<WireGuardConfig> WireGuardConfigParser::parse_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) return std::nullopt;

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse_string(buffer.str());
}

} // namespace fissio

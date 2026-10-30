#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <asio.hpp>

namespace fissio {

struct DohResult {
    std::string ip;
    std::vector<uint8_t> ech_config_list;
    bool has_ech = false;
    std::chrono::steady_clock::time_point expires_at;
};

class DohClient {
public:
    DohClient(asio::io_context& ioc, std::string doh_server_ip = "1.1.1.1", uint16_t doh_port = 443);

    // Asynchronous domain ECH and IP resolution
    asio::awaitable<std::optional<DohResult>> resolve(std::string domain);

    // Helpers to build and parse DNS Wire Format (testable)
    static std::vector<uint8_t> build_wire_query(const std::string& domain, uint16_t qtype, uint16_t tx_id = 0x1234);
    static bool parse_wire_response(const uint8_t* data, size_t len,
                                    std::vector<std::string>& out_ips,
                                    std::vector<uint8_t>& out_ech_config);

private:
    asio::io_context& ioc_;
    std::string doh_ip_;
    uint16_t doh_port_;

    // In-memory cache (TTL tracking)
    std::unordered_map<std::string, DohResult> cache_;
    std::mutex cache_mutex_;

    std::optional<DohResult> get_from_cache(const std::string& domain);
    void save_to_cache(const std::string& domain, const DohResult& res);
};

} // namespace fissio

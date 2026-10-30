#pragma once

#include <string>

namespace fissio {

class SystemProxy {
public:
    // Enable Windows system SOCKS5 proxy (127.0.0.1:port)
    static bool enable_socks5(uint16_t port);

    // Disable Windows system proxy
    static bool disable();

    // Check if proxy is currently enabled
    static bool is_enabled();
};

} // namespace fissio

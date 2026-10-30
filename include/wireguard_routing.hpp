#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace fissio {

struct TunnelConfig {
    std::string client_ip = "10.66.66.2";         // Client virtual IP address
    UINT8 prefix_len = 24;                        // Subnet prefix length (24 = 255.255.255.0)
    std::string virtual_gateway = "10.66.66.1";   // Server virtual IP / Gateway
    std::string server_endpoint = "";             // Server public IP (loaded dynamically from config)
    ULONG mtu = 1360;                             // Recommended MTU (Prevents short buffer: 1360 or 1420)
};

class WireGuardRouteController {
public:
    WireGuardRouteController();
    ~WireGuardRouteController();

    // Prevent copying (RAII resource safety)
    WireGuardRouteController(const WireGuardRouteController&) = delete;
    WireGuardRouteController& operator=(const WireGuardRouteController&) = delete;

    /**
     * @brief [Called on tunnel start] Force MTU, bind virtual IP, inject 3-stage routing rules
     * @param wintunLuid NET_LUID of the Wintun adapter
     * @param config Tunnel configuration (Client IP, Endpoint, Gateway, MTU)
     * @return Success status (auto-rollback on failure)
     */
    bool StartTunnel(NET_LUID wintunLuid, const TunnelConfig& config);

    /**
     * @brief [Called on tunnel stop] Delete injected routes, remove virtual IP, restore network
     */
    void StopTunnel();

    bool IsActive() const { return is_active_; }

private:
    bool SetMtu(NET_LUID luid, ULONG mtu);
    bool AssignUnicastIp(NET_LUID luid, const std::string& ip, UINT8 prefix);
    bool FindPhysicalGateway(const std::string& serverIp, NET_LUID& outLuid, SOCKADDR_INET& outGw);
    bool AddRoute(NET_LUID luid, const std::string& dest, UINT8 prefix,
                  const SOCKADDR_INET* nextHop, ULONG metric, MIB_IPFORWARD_ROW2& outRow);

    bool is_active_ = false;

    // Rollback state tracking
    MIB_UNICASTIPADDRESS_ROW ip_row_{};
    bool ip_assigned_ = false;

    std::vector<MIB_IPFORWARD_ROW2> active_routes_;
};

} // namespace fissio

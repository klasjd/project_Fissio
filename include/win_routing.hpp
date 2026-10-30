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

class WinRoutingManager {
public:
    WinRoutingManager();
    ~WinRoutingManager();

    WinRoutingManager(const WinRoutingManager&) = delete;
    WinRoutingManager& operator=(const WinRoutingManager&) = delete;

    /**
     * @brief Assign virtual IP, set MTU (1360), and inject 3-stage routing rules
     */
    bool apply_tunnel_routing(NET_LUID adapterLuid,
                             const std::string& clientIpStr,
                             UINT8 prefixLength,
                             const std::string& serverEndpointIpStr,
                             const std::string& gatewayIpStr,
                             ULONG mtu = 1360);

    /**
     * @brief Rollback injected routes and IP configuration
     */
    void rollback();

    bool is_active() const { return is_active_; }

private:
    bool assign_unicast_ip(NET_LUID luid, const std::string& ipStr, UINT8 prefixLen);
    bool set_interface_mtu(NET_LUID luid, ULONG mtu);
    bool find_physical_best_route(const std::string& targetIpStr,
                                  NET_LUID& outPhysicalLuid,
                                  SOCKADDR_INET& outPhysicalGateway);
    bool add_route(NET_LUID luid,
                   const std::string& destIpStr,
                   UINT8 prefixLen,
                   const SOCKADDR_INET* nextHop,
                   ULONG metric,
                   MIB_IPFORWARD_ROW2& outRow);

    bool is_active_ = false;
    MIB_UNICASTIPADDRESS_ROW client_ip_row_{};
    bool ip_assigned_ = false;
    std::vector<MIB_IPFORWARD_ROW2> injected_routes_;
};

} // namespace fissio

#include "win_routing.hpp"
#include <ws2tcpip.h>
#include <iostream>

namespace fissio {

WinRoutingManager::WinRoutingManager() = default;

WinRoutingManager::~WinRoutingManager() {
    rollback();
}

static bool parse_ipv4(const std::string& ipStr, IN_ADDR& outAddr) {
    return (inet_pton(AF_INET, ipStr.c_str(), &outAddr) == 1);
}

bool WinRoutingManager::assign_unicast_ip(NET_LUID luid, const std::string& ipStr, UINT8 prefixLen) {
    IN_ADDR inAddr{};
    if (!parse_ipv4(ipStr, inAddr)) {
        std::cerr << "[WinRouting] Invalid client IPv4 address: " << ipStr << "\n";
        return false;
    }

    InitializeUnicastIpAddressEntry(&client_ip_row_);
    client_ip_row_.InterfaceLuid = luid;
    client_ip_row_.Address.si_family = AF_INET;
    client_ip_row_.Address.Ipv4.sin_addr = inAddr;
    client_ip_row_.OnLinkPrefixLength = prefixLen;
    client_ip_row_.DadState = IpDadStatePreferred;

    DWORD err = CreateUnicastIpAddressEntry(&client_ip_row_);
    if (err != NO_ERROR && err != ERROR_OBJECT_ALREADY_EXISTS) {
        std::cerr << "[WinRouting] CreateUnicastIpAddressEntry failed. ErrorCode: " << err << "\n";
        return false;
    }

    ip_assigned_ = true;
    std::cout << "[WinRouting] Assigned virtual IP: " << ipStr << "/" << (int)prefixLen << "\n";
    return true;
}

bool WinRoutingManager::set_interface_mtu(NET_LUID luid, ULONG mtu) {
    MIB_IPINTERFACE_ROW ifRow{};
    InitializeIpInterfaceEntry(&ifRow);
    ifRow.InterfaceLuid = luid;
    ifRow.Family = AF_INET;

    DWORD err = GetIpInterfaceEntry(&ifRow);
    if (err != NO_ERROR) {
        std::cerr << "[WinRouting] GetIpInterfaceEntry failed. ErrorCode: " << err << "\n";
        return false;
    }

    ifRow.NlMtu = mtu;
    err = SetIpInterfaceEntry(&ifRow);
    if (err != NO_ERROR) {
        std::cerr << "[WinRouting] SetIpInterfaceEntry (MTU) failed. ErrorCode: " << err << "\n";
        return false;
    }

    std::cout << "[WinRouting] Configured Interface MTU=" << mtu << " via IP Helper API\n";
    return true;
}

bool WinRoutingManager::find_physical_best_route(const std::string& targetIpStr,
                                                NET_LUID& outPhysicalLuid,
                                                SOCKADDR_INET& outPhysicalGateway) {
    SOCKADDR_INET destAddr{};
    destAddr.si_family = AF_INET;
    if (!parse_ipv4(targetIpStr, destAddr.Ipv4.sin_addr)) {
        return false;
    }

    MIB_IPFORWARD_ROW2 bestRoute{};
    SOCKADDR_INET bestSourceAddr{};

    DWORD err = GetBestRoute2(NULL, 0, NULL, &destAddr, 0, &bestRoute, &bestSourceAddr);
    if (err != NO_ERROR) {
        std::cerr << "[WinRouting] GetBestRoute2 failed for " << targetIpStr << ". ErrorCode: " << err << "\n";
        return false;
    }

    outPhysicalLuid = bestRoute.InterfaceLuid;
    outPhysicalGateway = bestRoute.NextHop;
    return true;
}

bool WinRoutingManager::add_route(NET_LUID luid,
                                 const std::string& destIpStr,
                                 UINT8 prefixLen,
                                 const SOCKADDR_INET* nextHop,
                                 ULONG metric,
                                 MIB_IPFORWARD_ROW2& outRow) {
    InitializeIpForwardEntry(&outRow);
    outRow.InterfaceLuid = luid;
    outRow.DestinationPrefix.Prefix.si_family = AF_INET;
    parse_ipv4(destIpStr, outRow.DestinationPrefix.Prefix.Ipv4.sin_addr);
    outRow.DestinationPrefix.PrefixLength = prefixLen;

    if (nextHop) {
        outRow.NextHop = *nextHop;
    } else {
        outRow.NextHop.si_family = AF_INET;
        outRow.NextHop.Ipv4.sin_addr.s_addr = INADDR_ANY;
    }

    outRow.Metric = metric;
    outRow.Protocol = MIB_IPPROTO_NETMGMT;

    DWORD err = CreateIpForwardEntry2(&outRow);
    if (err != NO_ERROR && err != ERROR_OBJECT_ALREADY_EXISTS) {
        std::cerr << "[WinRouting] CreateIpForwardEntry2 failed for "
                  << destIpStr << "/" << (int)prefixLen << ". ErrorCode: " << err << "\n";
        return false;
    }

    installed_routes_.push_back(outRow);
    std::cout << "[WinRouting] Route added: " << destIpStr << "/" << (int)prefixLen
              << " (Metric: " << metric << ")\n";
    return true;
}

bool WinRoutingManager::apply_tunnel_routing(NET_LUID adapterLuid,
                                            const std::string& clientIpStr,
                                            UINT8 prefixLength,
                                            const std::string& serverEndpointIpStr,
                                            const std::string& gatewayIpStr,
                                            ULONG mtu) {
    rollback();

    // 1. Assign virtual IP to Wintun adapter
    if (!assign_unicast_ip(adapterLuid, clientIpStr, prefixLength)) {
        rollback();
        return false;
    }

    // 2. Set MTU to 1360 (prevents packet drop and timeout)
    set_interface_mtu(adapterLuid, mtu);

    // 3. Detect physical NIC and default gateway
    NET_LUID physicalLuid{};
    SOCKADDR_INET physicalGateway{};
    if (!find_physical_best_route(serverEndpointIpStr, physicalLuid, physicalGateway)) {
        std::cerr << "[WinRouting] Failed to find physical default gateway for server.\n";
        rollback();
        return false;
    }

    // 4. Rule 1: Server IP /32 endpoint loop prevention
    MIB_IPFORWARD_ROW2 r1{};
    if (!add_route(physicalLuid, serverEndpointIpStr, 32, &physicalGateway, 5, r1)) {
        rollback();
        return false;
    }

    // 5. Rules 2 & 3: 0.0.0.0/1 & 128.0.0.0/1 split tunneling
    SOCKADDR_INET virtualGw{};
    virtualGw.si_family = AF_INET;
    parse_ipv4(gatewayIpStr, virtualGw.Ipv4.sin_addr);

    MIB_IPFORWARD_ROW2 r2{}, r3{};
    if (!add_route(adapterLuid, "0.0.0.0", 1, &virtualGw, 1, r2) ||
        !add_route(adapterLuid, "128.0.0.0", 1, &virtualGw, 1, r3)) {
        rollback();
        return false;
    }

    is_active_ = true;
    std::cout << "[WinRouting] Tunnel routing successfully configured (MTU: " << mtu << ")!\n";
    return true;
}

void WinRoutingManager::rollback() {
    for (auto it = installed_routes_.rbegin(); it != installed_routes_.rend(); ++it) {
        DeleteIpForwardEntry2(&(*it));
    }
    installed_routes_.clear();

    if (ip_assigned_) {
        DeleteUnicastIpAddressEntry(&client_ip_row_);
        ip_assigned_ = false;
    }

    is_active_ = false;
}

} // namespace fissio

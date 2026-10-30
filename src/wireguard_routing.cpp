#include "wireguard_routing.hpp"
#include <ws2tcpip.h>
#include <iostream>

namespace fissio {

static bool StrToIpv4(const std::string& ipStr, IN_ADDR& outAddr) {
    return InetPtonA(AF_INET, ipStr.c_str(), &outAddr) == 1;
}

WireGuardRouteController::WireGuardRouteController() = default;

WireGuardRouteController::~WireGuardRouteController() {
    StopTunnel();
}

// 1. Force MTU to 1360 (or 1420) on Wintun adapter (prevents 'short buffer' caused by default 65535)
bool WireGuardRouteController::SetMtu(NET_LUID luid, ULONG mtu) {
    MIB_IPINTERFACE_ROW ifRow{};
    InitializeIpInterfaceEntry(&ifRow);
    ifRow.InterfaceLuid = luid;
    ifRow.Family = AF_INET;

    DWORD err = GetIpInterfaceEntry(&ifRow);
    if (err != NO_ERROR) {
        std::cerr << "[RouteController] GetIpInterfaceEntry failed: " << err << "\n";
        return false;
    }

    ifRow.NlMtu = mtu;
    err = SetIpInterfaceEntry(&ifRow);
    if (err != NO_ERROR) {
        std::cerr << "[RouteController] SetIpInterfaceEntry (MTU=" << mtu << ") failed: " << err << "\n";
        return false;
    }

    std::cout << "[RouteController] Wintun MTU forced to " << mtu << " (Short-buffer prevention OK)\n";
    return true;
}

// 2. Statically assign client virtual IP (10.66.66.2/24) to Wintun adapter
bool WireGuardRouteController::AssignUnicastIp(NET_LUID luid, const std::string& ip, UINT8 prefix) {
    IN_ADDR inAddr{};
    if (!StrToIpv4(ip, inAddr)) {
        std::cerr << "[RouteController] Invalid IPv4 address: " << ip << "\n";
        return false;
    }

    InitializeUnicastIpAddressEntry(&ip_row_);
    ip_row_.InterfaceLuid = luid;
    ip_row_.Address.si_family = AF_INET;
    ip_row_.Address.Ipv4.sin_addr = inAddr;
    ip_row_.OnLinkPrefixLength = prefix;
    ip_row_.DadState = IpDadStatePreferred;

    DWORD err = CreateUnicastIpAddressEntry(&ip_row_);
    if (err != NO_ERROR && err != ERROR_OBJECT_ALREADY_EXISTS) {
        std::cerr << "[RouteController] CreateUnicastIpAddressEntry failed: " << err << "\n";
        return false;
    }

    ip_assigned_ = true;
    std::cout << "[RouteController] Virtual IP assigned: " << ip << "/" << (int)prefix << "\n";
    return true;
}

// 3. Auto-detect active physical NIC and router gateway reaching the server endpoint IP
bool WireGuardRouteController::FindPhysicalGateway(const std::string& serverIp, NET_LUID& outLuid, SOCKADDR_INET& outGw) {
    SOCKADDR_INET dest{};
    dest.si_family = AF_INET;
    if (!StrToIpv4(serverIp, dest.Ipv4.sin_addr)) {
        return false;
    }

    MIB_IPFORWARD_ROW2 bestRoute{};
    SOCKADDR_INET bestSrc{};

    DWORD err = GetBestRoute2(NULL, 0, NULL, &dest, 0, &bestRoute, &bestSrc);
    if (err != NO_ERROR) {
        std::cerr << "[RouteController] GetBestRoute2 failed for server endpoint " << serverIp << ": " << err << "\n";
        return false;
    }

    outLuid = bestRoute.InterfaceLuid;
    outGw = bestRoute.NextHop; // Physical gateway (e.g. 192.168.219.1)
    return true;
}

// Common helper to add route entry via CreateIpForwardEntry2
bool WireGuardRouteController::AddRoute(NET_LUID luid, const std::string& dest, UINT8 prefix,
                                       const SOCKADDR_INET* nextHop, ULONG metric, MIB_IPFORWARD_ROW2& outRow) {
    InitializeIpForwardEntry(&outRow);
    outRow.InterfaceLuid = luid;
    outRow.DestinationPrefix.Prefix.si_family = AF_INET;
    StrToIpv4(dest, outRow.DestinationPrefix.Prefix.Ipv4.sin_addr);
    outRow.DestinationPrefix.PrefixLength = prefix;

    if (nextHop) {
        outRow.NextHop = *nextHop;
    } else {
        outRow.NextHop.si_family = AF_INET;
        outRow.NextHop.Ipv4.sin_addr.s_addr = INADDR_ANY;
    }

    outRow.Metric = metric;
    outRow.Protocol = MIB_IPPROTO_NETMGMT; // Management / software created flag

    DWORD err = CreateIpForwardEntry2(&outRow);
    if (err != NO_ERROR && err != ERROR_OBJECT_ALREADY_EXISTS) {
        std::cerr << "[RouteController] CreateIpForwardEntry2 failed for " << dest << "/" << (int)prefix
                  << ": " << err << "\n";
        return false;
    }

    active_routes_.push_back(outRow);
    std::cout << "[RouteController] Route added: " << dest << "/" << (int)prefix
              << " (Metric: " << metric << ")\n";
    return true;
}

// [Called on tunnel start]
bool WireGuardRouteController::StartTunnel(NET_LUID wintunLuid, const TunnelConfig& config) {
    StopTunnel(); // Clean up existing entries if any

    // Step 1: Set MTU to 1360 (or 1420)
    if (!SetMtu(wintunLuid, config.mtu)) {
        StopTunnel();
        return false;
    }

    // Step 2: Assign virtual IP (10.66.66.2/24)
    if (!AssignUnicastIp(wintunLuid, config.client_ip, config.prefix_len)) {
        StopTunnel();
        return false;
    }

    // Step 3: Detect physical NIC and default router gateway (192.168.219.1)
    NET_LUID physLuid{};
    SOCKADDR_INET physGw{};
    if (!FindPhysicalGateway(config.server_endpoint, physLuid, physGw)) {
        std::cerr << "[RouteController] Physical gateway detection failed.\n";
        StopTunnel();
        return false;
    }

    // Step 4: Rule 1 (Server public IP /32 endpoint loop prevention)
    //         ServerEndpoint/32 -> Physical NIC / Router Gateway (Metric: 5)
    MIB_IPFORWARD_ROW2 r1{};
    if (!AddRoute(physLuid, config.server_endpoint, 32, &physGw, 5, r1)) {
        StopTunnel();
        return false;
    }

    // Step 5: Rules 2 & 3 (Full internet 0/1 split route)
    //         0.0.0.0/1 & 128.0.0.0/1 -> Wintun adapter / 10.66.66.1 (Metric: 1)
    SOCKADDR_INET virtGw{};
    virtGw.si_family = AF_INET;
    StrToIpv4(config.virtual_gateway, virtGw.Ipv4.sin_addr);

    MIB_IPFORWARD_ROW2 r2{}, r3{};
    if (!AddRoute(wintunLuid, "0.0.0.0", 1, &virtGw, 1, r2) ||
        !AddRoute(wintunLuid, "128.0.0.0", 1, &virtGw, 1, r3)) {
        StopTunnel();
        return false;
    }

    is_active_ = true;
    std::cout << "[RouteController] All tunnel routes successfully injected via Win32 IP Helper API!\n";
    return true;
}

// [Called on tunnel stop]
void WireGuardRouteController::StopTunnel() {
    // 1. Safely delete injected routes (0.0.0.0/1, 128.0.0.0/1, serverIP/32) in reverse order
    for (auto it = active_routes_.rbegin(); it != active_routes_.rend(); ++it) {
        DWORD err = DeleteIpForwardEntry2(&(*it));
        if (err == NO_ERROR) {
            std::cout << "[RouteController] Route deleted successfully.\n";
        }
    }
    active_routes_.clear();

    // 2. Unbind virtual IP from adapter
    if (ip_assigned_) {
        DeleteUnicastIpAddressEntry(&ip_row_);
        ip_assigned_ = false;
        std::cout << "[RouteController] Virtual IP unassigned.\n";
    }

    is_active_ = false;
}

} // namespace fissio

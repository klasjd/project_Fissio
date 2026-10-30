#include <iostream>
#include <string>
#include <asio.hpp>
#include "proxy_session.hpp"

#if defined(_WIN32)
#include <winsock2.h>
#endif

int main(int argc, char* argv[]) {
#if defined(_WIN32)
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

    uint16_t port = 10808; // Default SOCKS5 port
    fissio::FragmentConfig frag_cfg;
    frag_cfg.split_size = 5; // Initial 5 bytes split
    frag_cfg.delay_ms = 3;   // 3ms inter-packet delay
    frag_cfg.enable_fragment = true;

    // CLI option parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if ((arg == "-s" || arg == "--split") && i + 1 < argc) {
            frag_cfg.split_size = static_cast<size_t>(std::stoi(argv[++i]));
        } else if ((arg == "-d" || arg == "--delay") && i + 1 < argc) {
            frag_cfg.delay_ms = std::stoi(argv[++i]);
        } else if (arg == "--no-frag") {
            frag_cfg.enable_fragment = false;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Fissio - ECH & TCP Fragmented SOCKS5 Proxy\n"
                      << "Usage: fissio [options]\n\n"
                      << "Options:\n"
                      << "  -p, --port <port>     SOCKS5 listen port (default: 10808)\n"
                      << "  -s, --split <bytes>   TCP fragmentation split size (default: 5 bytes)\n"
                      << "  -d, --delay <ms>      Inter-packet delay in milliseconds (default: 3 ms)\n"
                      << "  --no-frag             Disable TCP fragmentation\n"
                      << "  -h, --help            Show this help message\n";
            return 0;
        }
    }

    std::cout << "========================================================\n"
              << "   Fissio: Ultra-lightweight ECH & TCP Fragment Proxy   \n"
              << "========================================================" << std::endl;

    try {
        asio::io_context ioc(1);

        // Handle Ctrl+C signal
        asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) {
            std::cout << "\n[Fissio] Shutting down...\n";
            ioc.stop();
        });

        fissio::ProxyServer server(ioc, port, frag_cfg);
        server.start();

        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "[Fissio Fatal Error] " << e.what() << "\n";
    }

#if defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}

#pragma once

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
#else
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
using socket_t = int;
#endif

#include <openssl/base.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace fissio {

struct FragmentConfig {
    size_t split_size = 5;      // 1st stage packet split size (2~5 bytes)
    int delay_ms = 3;           // Inter-packet delay in ms (0~5 ms)
    bool enable_fragment = true;// Enable TCP fragmentation
};

class FragmentBioContext {
public:
    socket_t sock;
    FragmentConfig config;
    bool client_hello_split_done = false;
    SSL* ssl = nullptr;

    FragmentBioContext(socket_t s, FragmentConfig cfg)
        : sock(s), config(cfg) {}
};

class TlsSessionManager {
public:
    static SSL_CTX* create_client_context();
    static void destroy_context(SSL_CTX* ctx);

    // Create SSL session configured with ECH and custom BIO
    static SSL* create_ech_ssl(SSL_CTX* ctx, socket_t sock,
                               const std::string& host,
                               const std::vector<uint8_t>& ech_config_list,
                               FragmentConfig frag_cfg = {});

    // Manual TCP fragmentation for raw sockets (browser ClientHello binary split)
    static bool send_fragmented_client_hello(socket_t sock, const uint8_t* data, size_t len,
                                             const FragmentConfig& cfg);

private:
    static BIO_METHOD* get_fragment_bio_method();
};

} // namespace fissio

#include "tls_fragment.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace fissio {

static void set_socket_nodelay(socket_t sock) {
    int flag = 1;
#if defined(_WIN32)
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag));
#else
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
#endif
}

static int bio_fragment_write(BIO* bio, const char* in, int inl) {
    auto* ctx = static_cast<FragmentBioContext*>(BIO_get_data(bio));
    if (!ctx || inl <= 0) return 0;

    BIO_clear_retry_flags(bio);

    // Handshake not finished yet and initial ClientHello fragmentation is needed
    if (ctx->config.enable_fragment && !ctx->client_hello_split_done) {
        if (!ctx->ssl || !SSL_is_init_finished(ctx->ssl)) {
            set_socket_nodelay(ctx->sock);

            size_t split = (static_cast<size_t>(inl) > ctx->config.split_size)
                           ? ctx->config.split_size : static_cast<size_t>(inl);

            // Step 1: Send initial slice (2~5 bytes)
#if defined(_WIN32)
            int sent1 = send(ctx->sock, in, static_cast<int>(split), 0);
#else
            int sent1 = static_cast<int>(send(ctx->sock, in, split, 0));
#endif
            if (sent1 <= 0) {
                BIO_set_retry_write(bio);
                return sent1;
            }

            if (static_cast<size_t>(sent1) < static_cast<size_t>(inl)) {
                // Step 2: Inter-packet delay (1~5ms)
                if (ctx->config.delay_ms > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(ctx->config.delay_ms));
                }

                // Step 3: Send remaining bytes
                const char* remaining = in + sent1;
                int remaining_len = inl - sent1;
#if defined(_WIN32)
                int sent2 = send(ctx->sock, remaining, remaining_len, 0);
#else
                int sent2 = static_cast<int>(send(ctx->sock, remaining, remaining_len, 0));
#endif
                if (sent2 < 0) {
                    return sent1;
                }
                ctx->client_hello_split_done = true;
                return sent1 + sent2;
            }

            ctx->client_hello_split_done = true;
            return sent1;
        }
    }

    // Direct transmission without delay after initial handshake
#if defined(_WIN32)
    int ret = send(ctx->sock, in, inl, 0);
#else
    int ret = static_cast<int>(send(ctx->sock, in, inl, 0));
#endif
    if (ret <= 0) {
        BIO_set_retry_write(bio);
    }
    return ret;
}

static int bio_fragment_read(BIO* bio, char* out, int outl) {
    auto* ctx = static_cast<FragmentBioContext*>(BIO_get_data(bio));
    if (!ctx || outl <= 0) return 0;

    BIO_clear_retry_flags(bio);
#if defined(_WIN32)
    int ret = recv(ctx->sock, out, outl, 0);
#else
    int ret = static_cast<int>(recv(ctx->sock, out, outl, 0));
#endif
    if (ret <= 0) {
        BIO_set_retry_read(bio);
    }
    return ret;
}

static long bio_fragment_ctrl(BIO* bio, int cmd, long num, void* ptr) {
    switch (cmd) {
    case BIO_CTRL_FLUSH:
        return 1;
    default:
        return 0;
    }
}

static int bio_fragment_create(BIO* bio) {
    BIO_set_init(bio, 1);
    BIO_set_data(bio, nullptr);
    return 1;
}

static int bio_fragment_destroy(BIO* bio) {
    if (!bio) return 0;
    auto* ctx = static_cast<FragmentBioContext*>(BIO_get_data(bio));
    delete ctx;
    BIO_set_data(bio, nullptr);
    BIO_set_init(bio, 0);
    return 1;
}

BIO_METHOD* TlsSessionManager::get_fragment_bio_method() {
    static BIO_METHOD* method = []() {
        BIO_METHOD* m = BIO_meth_new(BIO_TYPE_SOURCE_SINK, "fissio_fragment_bio");
        BIO_meth_set_write(m, bio_fragment_write);
        BIO_meth_set_read(m, bio_fragment_read);
        BIO_meth_set_ctrl(m, bio_fragment_ctrl);
        BIO_meth_set_create(m, bio_fragment_create);
        BIO_meth_set_destroy(m, bio_fragment_destroy);
        return m;
    }();
    return method;
}

SSL_CTX* TlsSessionManager::create_client_context() {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) return nullptr;

    static const uint8_t alpn[] = {8, 'h', 't', 't', 'p', '/', '1', '.', '1'};
    SSL_CTX_set_alpn_protos(ctx, alpn, sizeof(alpn));

    return ctx;
}

void TlsSessionManager::destroy_context(SSL_CTX* ctx) {
    if (ctx) SSL_CTX_free(ctx);
}

SSL* TlsSessionManager::create_ech_ssl(SSL_CTX* ctx, socket_t sock,
                                      const std::string& host,
                                      const std::vector<uint8_t>& ech_config_list,
                                      FragmentConfig frag_cfg) {
    SSL* ssl = SSL_new(ctx);
    if (!ssl) return nullptr;

    SSL_set_tlsext_host_name(ssl, host.c_str());

    if (!ech_config_list.empty()) {
        if (!SSL_set1_ech_config_list(ssl, ech_config_list.data(), ech_config_list.size())) {
            std::cerr << "[TLS] Warning: Failed to set ECHConfigList for " << host << "\n";
        } else {
            std::cout << "[TLS] Successfully configured ECH for " << host << " (" << ech_config_list.size() << " bytes)\n";
        }
    }

    BIO* fragment_bio = BIO_new(get_fragment_bio_method());
    auto* bio_ctx = new FragmentBioContext(sock, frag_cfg);
    bio_ctx->ssl = ssl;
    BIO_set_data(fragment_bio, bio_ctx);

    SSL_set_bio(ssl, fragment_bio, fragment_bio);
    return ssl;
}

bool TlsSessionManager::send_fragmented_client_hello(socket_t sock, const uint8_t* data, size_t len,
                                                    const FragmentConfig& cfg) {
    if (len == 0) return true;

    set_socket_nodelay(sock);

    size_t split = (len > cfg.split_size) ? cfg.split_size : len;

#if defined(_WIN32)
    int s1 = send(sock, reinterpret_cast<const char*>(data), static_cast<int>(split), 0);
#else
    int s1 = static_cast<int>(send(sock, reinterpret_cast<const char*>(data), split, 0));
#endif
    if (s1 <= 0) return false;

    if (split < len) {
        if (cfg.delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.delay_ms));
        }

        const uint8_t* rest = data + split;
        size_t rest_len = len - split;
#if defined(_WIN32)
        int s2 = send(sock, reinterpret_cast<const char*>(rest), static_cast<int>(rest_len), 0);
#else
        int s2 = static_cast<int>(send(sock, reinterpret_cast<const char*>(rest), rest_len, 0));
#endif
        if (s2 <= 0) return false;
    }

    return true;
}

} // namespace fissio

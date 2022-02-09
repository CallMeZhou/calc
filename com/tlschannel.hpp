#pragma once
#include <string>
#include "channel.hpp"
#include "openssl/ssl.h"

namespace network_channel {
using namespace std;

class tls_channel : public base_channel {
    SSL *ossl;
public:
    tls_channel(int peerfd, SSL_CTX *ossl_ctx);
    ~tls_channel();
    ssize_t recv (void *buf, size_t n, int flags = 0) override;
    ssize_t send (const void *buf, size_t n, int flags = 0) override;

    static channel* factory(int peerfd, SSL_CTX *ossl_ctx);
};

class tls_contex {
    SSL_CTX *ossl_ctx;
public:
    tls_contex(const string &cert_file, const string &key_file);
    ~tls_contex();
    operator SSL_CTX* ();
};

}
#pragma once
#include <string>
#include "channel.hpp"
#include "openssl/ssl.h"

namespace network_channel {
using namespace std;

class tls_channel : public channel {
    int peerfd;
    SSL *ossl;
public:
    tls_channel(int peerfd, SSL_CTX *ossl_ctx);
    virtual ~tls_channel();
    virtual int get_fd(void) const;
    virtual ssize_t recv (void *buf, size_t n, int flags = 0);
    virtual ssize_t send (const void *buf, size_t n, int flags = 0);

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
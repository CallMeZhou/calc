#pragma once
#include <string>
#include "channel.hpp"
#include "openssl/ssl.h"

namespace network_channel {
using namespace std;

class tls_channel : public channel {
    int peerfd;
    SSL_CTX *ossl_ctx;
    SSL *ossl;
public:
    tls_channel(int peerfd, const string &cert_file, const string &key_file);
    virtual ~tls_channel();
    virtual int get_fd(void) const;
    virtual ssize_t recv (void *buf, size_t n, int flags = 0);
    virtual ssize_t send (const void *buf, size_t n, int flags = 0);

    static channel* factory(int peerfd, const string &cert_file, const string &key_file);
};

}
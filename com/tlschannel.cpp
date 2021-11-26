#include <unistd.h>
#include <sys/socket.h>
#include <cassert>
#include <sstream>
#include <utility>
#include "openssl/err.h"
#include "excepts.hpp"
#include "tlschannel.hpp"

namespace server_excepts {
using namespace std;

tls_exception::tls_exception(void) : runtime_error("") {
    stringstream ss;
    ERR_print_errors_cb([](const char *str, size_t len, void *u) -> int {
        stringstream &ss = *((stringstream*)u);
        ss << str << endl;
        return 0;
    }, &ss);
    msg = ss.str();
}

const char* tls_exception::what(void) const noexcept {
    return msg.c_str();
}

}

namespace network_channel {
using namespace std;
using namespace server_excepts;

static const int TIMEOUT = 10; // timeout for peer socket. 10 seconds.

tls_channel::tls_channel(int peerfd, SSL_CTX *ossl_ctx)
: peerfd(0), ossl(nullptr) {
    struct timeval tv = {0};
    tv.tv_sec = TIMEOUT;
    setsockopt(peerfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    try {
        if (nullptr == (ossl = SSL_new(ossl_ctx))) {
            throw tls_exception();
        }

        if(0 == SSL_set_fd(ossl, peerfd)) {
            throw tls_exception();
        }

        if(1 != SSL_accept(ossl)) {
            throw tls_exception();
        }
    } catch (tls_exception &e) {
        if (ossl) SSL_free(ossl);
        throw e;
    }
}

tls_channel::~tls_channel() {
    if (ossl) {
        SSL_shutdown(ossl);
        SSL_free(ossl);
    }
}

int tls_channel::get_timeout(void) const {
    return TIMEOUT;
}

int tls_channel::get_fd(void) const {
    return peerfd;
}

ssize_t tls_channel::recv(void *buf, size_t n, int flags) {
    assert(flags == 0);
    int r = SSL_read(ossl, buf, n);
    int e = r > 0 ? 0 : SSL_get_error(ossl, r);
    if (r > 0 || SSL_ERROR_ZERO_RETURN == e) {
        return r;
    } else if (SSL_ERROR_WANT_READ == e) {
        errno = EAGAIN;
        return -1;
    } else {
        throw tls_exception();
    }
}

ssize_t tls_channel::send(const void *buf, size_t n, int flags) {
    assert(flags == 0);
    int w = SSL_write(ossl, buf, n);
    int e = w > 0 ? 0 : SSL_get_error(ossl, w);
    if (w > 0 || SSL_ERROR_ZERO_RETURN == e) {
        return w;
    } else if (SSL_ERROR_WANT_READ == e) {
        errno = EAGAIN;
        return -1;
    } else {
        throw tls_exception();
    }
}

channel* tls_channel::factory(int peerfd, SSL_CTX *ossl_ctx) {
    return new tls_channel(peerfd, ossl_ctx);
}

tls_contex::tls_contex(const string &cert_file, const string &key_file) : ossl_ctx(nullptr) {
    try {
        if (nullptr == (ossl_ctx = SSL_CTX_new(TLS_server_method()))) {
            throw tls_exception();
        }

        if(SSL_CTX_use_certificate_file(ossl_ctx, cert_file.c_str(), SSL_FILETYPE_PEM) < 0) {
            throw tls_exception();
        }

        if(SSL_CTX_use_PrivateKey_file(ossl_ctx, key_file.c_str(), SSL_FILETYPE_PEM) < 0) {
            throw tls_exception();
        }

        if (1 != SSL_CTX_check_private_key(ossl_ctx)) {
            throw tls_exception();
        }
    } catch (tls_exception &e) {
        if (ossl_ctx) SSL_CTX_free(ossl_ctx);
        throw e;
    }
}

tls_contex::~tls_contex() {
    SSL_CTX_free(ossl_ctx);
}

tls_contex::operator SSL_CTX* () {
    return ossl_ctx;
}


}
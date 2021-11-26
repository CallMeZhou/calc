#include <unistd.h>
#include <sys/socket.h>
#include <utility>
#include "tcpchannel.hpp"

namespace network_channel {
using namespace std;

static const int TIMEOUT = 10; // timeout for peer socket. 10 seconds.

tcp_channel::tcp_channel(int peerfd) : peerfd(peerfd) {
    struct timeval tv = {0};
    tv.tv_sec = TIMEOUT;
    setsockopt(peerfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

tcp_channel::~tcp_channel() {
    close(peerfd);
}

int tcp_channel::get_timeout(void) const {
    return TIMEOUT;
}

int tcp_channel::get_fd(void) const {
    return peerfd;
}

ssize_t tcp_channel::recv (void *buf, size_t n, int flags) {
    return ::recv(peerfd, buf, n, flags);
}

ssize_t tcp_channel::send (const void *buf, size_t n, int flags) {
    return ::send(peerfd, buf, n, flags);    
}

channel* tcp_channel::factory(int peerfd) {
    return new tcp_channel(peerfd);
}

}
#include <unistd.h>
#include <sys/socket.h>
#include <utility>
#include "tcpchannel.hpp"

namespace network_channel {
using namespace std;

tcp_channel::tcp_channel(int peer_fd) : base_channel(peer_fd) {
    struct timeval tv = {0};
    tv.tv_sec = inf.io_timeout;
    setsockopt(inf.peer_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

tcp_channel::~tcp_channel() {
    close(inf.peer_fd);
}

ssize_t tcp_channel::recv (void *buf, size_t n, int flags) {
    return ::recv(inf.peer_fd, buf, n, flags);
}

ssize_t tcp_channel::send (const void *buf, size_t n, int flags) {
    return ::send(inf.peer_fd, buf, n, flags);    
}

channel* tcp_channel::factory(int peer_fd) {
    return new tcp_channel(peer_fd);
}

}
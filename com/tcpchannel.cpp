#include <unistd.h>
#include <utility>
#include <sys/socket.h>
#include "tcpchannel.hpp"

namespace network_channel {
using namespace std;

tcp_channel::tcp_channel(int peerfd) : peerfd(peerfd) {
}

tcp_channel::~tcp_channel() {
    close(peerfd);
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
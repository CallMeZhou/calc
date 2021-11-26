#pragma once
#include "channel.hpp"

namespace network_channel {

class tcp_channel : public channel {
    int peerfd;
public:
    tcp_channel(int peerfd);
    virtual ~tcp_channel();
    virtual int get_timeout(void) const override;
    virtual int get_fd(void) const override;
    virtual ssize_t recv (void *buf, size_t n, int flags = 0) override;
    virtual ssize_t send (const void *buf, size_t n, int flags = 0) override;

    static channel* factory(int peerfd);
};

}
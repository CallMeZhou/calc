#pragma once
#include "channel.hpp"

namespace network_channel {

class tcp_channel : public base_channel {
public:
    tcp_channel(int peerfd);
    ~tcp_channel();
    ssize_t recv (void *buf, size_t n, int flags = 0) override;
    ssize_t send (const void *buf, size_t n, int flags = 0) override;

    static channel* factory(int peerfd);
};

}
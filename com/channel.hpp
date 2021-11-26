#pragma once
#include <sys/types.h>

namespace network_channel {

struct channel {
    virtual ~channel() {}
    virtual int get_timeout(void) const = 0;
    virtual int get_fd(void) const = 0;
    virtual ssize_t recv (void *buf, size_t n, int flags = 0) = 0;
    virtual ssize_t send (const void *buf, size_t n, int flags = 0) = 0;
};

}
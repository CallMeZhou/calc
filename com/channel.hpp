#pragma once
#include <sys/types.h>
#include <chrono>

namespace network_channel {

struct channel {
    struct info {
        int peer_fd;
        int io_timeout;
        int idol_timeout;
        std::chrono::steady_clock::time_point idol_start;
    };
    
    virtual ~channel() {}
    virtual const info& get_info(void) const = 0;
    virtual void set_idol_start(const std::chrono::steady_clock::time_point &start) = 0;
    virtual ssize_t recv (void *buf, size_t n, int flags = 0) = 0;
    virtual ssize_t send (const void *buf, size_t n, int flags = 0) = 0;
};

struct base_channel : public channel {
    constexpr static int IO_TIMEOUT   = 10; // timeout for peer socket io.   10 seconds.
    constexpr static int IDOL_TIMEOUT = 30; // timeout for peer socket idol. 30 seconds.
    info inf;
    base_channel(int peer_fd) {
        inf.peer_fd = peer_fd;
        inf.io_timeout = IO_TIMEOUT;
        inf.idol_timeout = IDOL_TIMEOUT;
    }
    const info& get_info(void) const override {
        return inf;
    }
    void set_idol_start(const std::chrono::steady_clock::time_point &start) override {
        inf.idol_start = start;
    }
};

}
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <array>
#include <vector>
#include "tcpsvr.hpp"
#include "utils.hpp"
#include "thrdpool.hpp"
#include "fmt/core.h"

namespace server {

tcp::tcp(const string &serivce) : sock(-1), stopping(false) {
    int err;

    struct addrinfo req = {0};
    req.ai_family = AF_INET;
    req.ai_socktype = SOCK_STREAM;
    req.ai_flags = AI_PASSIVE;

    struct addrinfo *addrs;
    if ((err = getaddrinfo(NULL, serivce.c_str(), &req, &addrs)) != 0) {
        throw runtime_error(gai_strerror(err));
    }

    for (struct addrinfo *addr = addrs; addr; addr = addr->ai_next) {
        if ((sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1) {
            continue;
        }

        if (bind(sock, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }

        close(sock);
        sock = -1;
    }

    freeaddrinfo(addrs);

    if (sock == -1) {
        throw runtime_error("Failed to bind");
    }
}

tcp::~tcp() {
    if (sock != -1) {
        offline();
    }
}

void tcp::online(function<void(int, const string &)> app_protocol, int max_concurrency) {
    assert(sock != -1);
    assert(!listener.joinable());

    listener = thread([this, app_protocol, max_concurrency]() {
        server_utils::thread_pool handler_pool(max_concurrency);

        while (!stopping) {
            if (listen(sock, listen_backlog) == -1) {
                if (!stopping) perror("listen");
                break;
            }

            struct sockaddr peerAddr;
            socklen_t peerAddrLen = sizeof(peerAddr);
            int peer = accept(sock, &peerAddr, &peerAddrLen);

            if (peer == -1) {
                if (stopping) break;
                perror("accept");
                continue;
            }

            char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
            getnameinfo(&peerAddr, peerAddrLen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
            string session_name = fmt::format("{}/{}", hbuf, sbuf); // just a name for debug purpose

            handler_pool.execute([app_protocol, peer, session_name](){ app_protocol(peer, session_name); });
        }
    });
}

void tcp::offline(void) {
    assert(sock != -1);

    stopping = true;
    shutdown(sock, SHUT_RDWR);
    sock = -1;

    if (listener.joinable()) {
        listener.join();
    }
}

}
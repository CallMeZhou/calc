#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
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
#include "excepts.hpp"
#include "fmt/core.h"

namespace server {
using namespace std;
using namespace server_excepts;
constexpr int MAX_EPOLLEVS = 16;

static tuple<int, string> my_accept(int listen_sock);
static void set_fd_nonblock(int fd);
static void add_fd_to_epoll(int epfd, int newfd, int events = EPOLLIN | EPOLLONESHOT);

void tcp::add_new_peer(int peer_fd, const string &name) {
    assert(connection_map.find(peer_fd) == connection_map.end());
    connection_map[peer_fd] = { name, shared_ptr<channel>(channel_from_fd(peer_fd)) };
}

tcp::connection& tcp::find_peer_by_fd(int peer_fd) {
    auto conn = connection_map.find(peer_fd);
    assert(conn != connection_map.end());
    return conn->second;
}

tcp::tcp(const string &serivce, function<channel*(int)> channel_factory)
: epfd(-1), listen_sock(-1), quit_event(-1), channel_from_fd(channel_factory) {
    int err;

    if ((quit_event = eventfd(0, EFD_NONBLOCK)) == -1) {
        throw runtime_error(strerror(err));
    }

    if ((epfd = epoll_create(1024)) == -1) {
        throw runtime_error(strerror(err));
    }

    add_fd_to_epoll(epfd, quit_event, EPOLLHUP | EPOLLERR | EPOLLIN);

    struct addrinfo req = {0};
    req.ai_family = AF_INET;
    req.ai_socktype = SOCK_STREAM;
    req.ai_flags = AI_PASSIVE;

    struct addrinfo *addrs;
    if ((err = getaddrinfo(NULL, serivce.c_str(), &req, &addrs)) != 0) {
        throw runtime_error(gai_strerror(err));
    }

    for (struct addrinfo *addr = addrs; addr; addr = addr->ai_next) {
        if ((listen_sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1) {
            continue;
        }

        if (bind(listen_sock, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }

        close(listen_sock);
        listen_sock = -1;
    }

    freeaddrinfo(addrs);

    if (listen_sock == -1) {
        throw runtime_error("Failed to bind");
    }

    set_fd_nonblock(listen_sock);
    add_fd_to_epoll(epfd, listen_sock);
}

tcp::~tcp() {
    if (listen_sock != -1) {
        offline();
    }

    if (epfd != -1) {
        close(epfd);
    }
}


//-------------------------------------------------------------
//>>>>>>>> TODO: peer_fd timeout and remove from epoll <<<<<<<<
//-------------------------------------------------------------


void tcp::online(function<void(channel*, const string &)> app_protocol, thread_pool &threads) {
    assert(listen_sock != -1);
    assert(!listener.joinable());

    listener = thread([this, app_protocol, &threads]() {
        bool stopping = false;
        while (!stopping) {
            epoll_event evs[MAX_EPOLLEVS];
            int nfds = epoll_wait(epfd, evs, MAX_EPOLLEVS, -1);

            if (-1 == nfds) {
                fmt::print("epoll_wait: {}\n", strerror(errno));
                break;
            } else {
                fmt::print("epoll_wait: {}\n", nfds);
            }

            for (int i = 0; i < nfds; i++) {
                try {
                    if (evs[i].data.fd == quit_event) {
                        fmt::print("stopping main server loop ... ");
                        stopping = true;
                        break;
                    } else if (evs[i].data.fd == listen_sock) {
                        int peer_fd;
                        string conn_name;
                        tie(peer_fd, conn_name) = my_accept(listen_sock);

                        deferred close_peer_fd_if_except_thrown ([peer_fd](void){ close (peer_fd); });

                        set_fd_nonblock(peer_fd);
                        add_fd_to_epoll(epfd, peer_fd);
                        add_new_peer(peer_fd, conn_name);

                        close_peer_fd_if_except_thrown.cancel();
                    } else {
                        auto conn = find_peer_by_fd(evs[i].data.fd);
                        threads.execute([app_protocol, conn](){ app_protocol(conn.chann.get(), conn.name); });
                    }
                } catch (exception &e) {
                    fmt::print(stderr, "{}\n", e.what());
                }
            }
        }

        if (stopping) fmt::print("done");
    });
}

void tcp::offline(void) {
    assert(listen_sock != -1);

    uint64_t unused = 0;
    auto w  = write(quit_event, &unused, sizeof(unused));
    fmt::print("write(quit_event): {}\n", w);

    if (listener.joinable()) {
        listener.join();
    }

    shutdown(listen_sock, SHUT_RDWR);
    listen_sock = -1;
}

static tuple<int, string> my_accept(int listen_sock) {
    struct sockaddr peerAddr;
    socklen_t peerAddrLen = sizeof(peerAddr);
    int peer_fd = accept(listen_sock, &peerAddr, &peerAddrLen);

    if (peer_fd == -1) {
        throw errno_exception("accept() failed");
    }

    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    getnameinfo(&peerAddr, peerAddrLen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);

    return {peer_fd, fmt::format("{}/{}", hbuf, sbuf)};
}

static void set_fd_nonblock(int fd) {
    int peer_fd_fl = fcntl(fd, F_GETFL);
    if (peer_fd_fl == -1) {
        throw errno_exception("fcntl(F_GETFL) failed", errno);
    }

    if (fcntl(fd, F_SETFL, peer_fd_fl | O_NONBLOCK) == -1) {
        throw errno_exception("fcntl(F_SETFL) failed", errno);
    }
}

static void add_fd_to_epoll(int epfd, int newfd, int events) {
    epoll_event ev;
    ev.events = events;
    ev.data.fd = newfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, newfd, &ev) == -1) {
        throw errno_exception("epoll_ctl(EPOLL_CTL_ADD) failed", errno);
    }
}

}

#include "tcpsvr.hpp"
#include "excepts.hpp"
#include "fmt/core.h"
#include "thrdpool.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <array>
#include <assert.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace server {

using namespace std;
using namespace server_excepts;

static tuple<int, string> my_accept(int listen_sock);
static void set_fd_nonblock(int fd);
static void add_fd_to_epoll(int ep_fd, int new_fd, int events);
static void update_fd_in_epoll(int ep_fd, int peer_fd, int events);
static void remove_fd_from_epoll(int ep_fd, int peer_fd);

void tcp::add_new_peer(int peer_fd, const string &name) {
    scoped_lock lock(connection_map_lock);
    assert(connection_map.find(peer_fd) == connection_map.end());
    connection_map[peer_fd] = {name, shared_ptr<channel>(channel_from_fd(peer_fd))};
}

tcp::connection &tcp::find_peer_by_fd(int peer_fd) {
    scoped_lock lock(connection_map_lock);
    auto conn = connection_map.find(peer_fd);
    assert(conn != connection_map.end());
    return conn->second;
}

string tcp::hangup_peer_by_fd(int peer_fd) {
    scoped_lock lock(connection_map_lock);
    auto conn = connection_map.find(peer_fd);
    assert(conn != connection_map.end());
    assert(conn->second.chann.use_count() == 1);
    string name = conn->second.name;
    connection_map.erase(conn);
    return name;
}

tcp::tcp(const string &serivce, function<channel *(int)> channel_factory)
    : ep_fd(-1), listen_sock(-1), quit_event(-1), channel_from_fd(channel_factory) {
    int err;

    if ((quit_event = eventfd(0, EFD_NONBLOCK)) == -1)
        throw runtime_error(strerror(err));
    if ((ep_fd = epoll_create1(0)) == -1)
        throw runtime_error(strerror(err));

    add_fd_to_epoll(ep_fd, quit_event, EPOLLIN);

    struct addrinfo req = { .ai_flags = AI_PASSIVE, .ai_family = AF_INET, .ai_socktype = SOCK_STREAM }, *addrs = nullptr;
    if ((err = getaddrinfo(NULL, serivce.c_str(), &req, &addrs)) != 0)
        throw runtime_error(gai_strerror(err));

    for (struct addrinfo *addr = addrs; addr; addr = addr->ai_next) {
        if ((listen_sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1)
            continue;

        deferred close_listen_sock_if_failed([this]() { close(listen_sock); listen_sock = -1; });

        if (bind(listen_sock, addr->ai_addr, addr->ai_addrlen) != 0)
            continue;
        if (listen(listen_sock, listen_backlog) != 0)
            continue;

        close_listen_sock_if_failed.cancel();
        break;
    }

    freeaddrinfo(addrs);

    if (listen_sock == -1)
        throw runtime_error("Failed to bind");

    add_fd_to_epoll(ep_fd, listen_sock, EPOLLIN);
}

tcp::~tcp() {
    if (listen_sock != -1) {
        offline();
    }

    if (ep_fd != -1) {
        close(ep_fd);
    }
}

//-------------------------------------------------------------
//>>>>>>>> TODO: peer_fd timeout and remove from epoll <<<<<<<<
//-------------------------------------------------------------

void tcp::online(function<void(channel *, const string &)> app_protocol, thread_pool &threads) {
    assert(listen_sock != -1);
    assert(!listener.joinable());

    listener = thread([this, app_protocol, &threads]() {
        bool stopping = false;
        while (!stopping) {
            epoll_event evs[max_epoll_events];
            int nfds = epoll_wait(ep_fd, evs, max_epoll_events, -1);

            if (-1 == nfds && errno != EINTR) {
                fmt::print("epoll_wait() failed due to '{}', tcp server's main thread {} is exiting...", strerror(errno), pthread_self());
                break;
            }

            for (int i = 0; i < nfds; i++) {
                auto fd_of_events = evs[i].data.fd;
                auto events = evs[i].events;

                try {
                    if (fd_of_events == quit_event) {

                        puts("stopping main server loop...");
                        stopping = true;
                        break;

                    } else if (fd_of_events == listen_sock) {

                        int peer_fd;
                        string conn_name;
                        tie(peer_fd, conn_name) = my_accept(listen_sock);

                        deferred close_peer_fd_if_except_thrown([peer_fd](void) { close(peer_fd); });

                        //set_fd_nonblock(peer_fd);
                        add_fd_to_epoll(ep_fd, peer_fd, EPOLLIN | EPOLLRDHUP | EPOLLONESHOT);
                        add_new_peer(peer_fd, conn_name);

                        fmt::print("client {} accepted on fd:{}.\n", conn_name, peer_fd);

                        close_peer_fd_if_except_thrown.cancel();

                    } else if (events & (EPOLLHUP | EPOLLRDHUP)) {

                        remove_fd_from_epoll(ep_fd, fd_of_events);
                        auto name = hangup_peer_by_fd(fd_of_events);
                        fmt::print("client {} (fd:{}) disconnected.\n", name, fd_of_events);

                    } else {

                        auto conn = find_peer_by_fd(fd_of_events);
                        threads.execute([this, app_protocol, &conn]() {
                            try {
                                app_protocol(conn.chann.get(), conn.name);
                                update_fd_in_epoll(ep_fd, conn.chann->get_fd(), EPOLLIN | EPOLLRDHUP | EPOLLONESHOT);
                            } catch (peer_completion &e) {
                                int fd = conn.chann->get_fd();
                                remove_fd_from_epoll(ep_fd, fd);
                                auto name = hangup_peer_by_fd(fd);
                                fmt::print("client {} (fd:{}) disconnected during request handling.\n", name, fd);
                            } catch (session_timeout &e) {
                                // TODO: to be implemented
                                assert(0);
                            }
                        });

                    }
                } catch (exception &e) {
                    fmt::print(stderr, "{}\n", e.what());
                }
            }
        }

        if (stopping)
            fmt::print("done");
    });
}

void tcp::offline(void) {
    assert(listen_sock != -1);

    if (eventfd_write(quit_event, 1) != 0)
        throw errno_exception("offline/eventfd_write() failed");

    if (listener.joinable())
        listener.join();

    shutdown(listen_sock, SHUT_RDWR);
    listen_sock = -1;
}

static tuple<int, string> my_accept(int listen_sock) {
    struct sockaddr peerAddr;
    socklen_t peerAddrLen = sizeof(peerAddr);
    int peer_fd = accept(listen_sock, &peerAddr, &peerAddrLen);

    if (peer_fd == -1)
        throw errno_exception("accept() failed");

    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    getnameinfo(&peerAddr, peerAddrLen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);

    return {peer_fd, fmt::format("{}/{}", hbuf, sbuf)};
}

static void set_fd_nonblock(int fd) {
    int peer_fd_fl = fcntl(fd, F_GETFL);
    if (peer_fd_fl == -1)
        throw errno_exception("fcntl(F_GETFL) failed", errno);

    if (fcntl(fd, F_SETFL, peer_fd_fl | O_NONBLOCK) == -1)
        throw errno_exception("fcntl(F_SETFL) failed", errno);
}

static void add_fd_to_epoll(int ep_fd, int new_fd, int events) {
    epoll_event ev;
    ev.events = events;
    ev.data.fd = new_fd;
    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, new_fd, &ev) == -1)
        throw errno_exception("epoll_ctl(EPOLL_CTL_ADD) failed", errno);
}

static void update_fd_in_epoll(int ep_fd, int peer_fd, int events) {
    epoll_event ev;
    ev.events = events;
    ev.data.fd = peer_fd;
    if (epoll_ctl(ep_fd, EPOLL_CTL_MOD, peer_fd, &ev) == -1)
        throw errno_exception("epoll_ctl(EPOLL_CTL_MOD) failed", errno);
}

static void remove_fd_from_epoll(int ep_fd, int peer_fd) {
    if (epoll_ctl(ep_fd, EPOLL_CTL_DEL, peer_fd, nullptr) == -1)
        throw errno_exception("epoll_ctl(EPOLL_CTL_DEL) failed", errno);    
}

}

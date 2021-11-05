#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <vector>
#include <stdexcept>
#include <thread>
#include <functional>
#include "protocol.hpp"
#include "server.hpp"

StreamServerHandler::StreamServerHandler(int peer) : peer(peer) {
    std::thread(&StreamServerHandler::handlerFunc, this);
}

StreamServerHandler::~StreamServerHandler() {
    close(peer);
}

void SimpleHandler::handlerFunc(void) const {
    std::vector<char> request, response;

    while (true) {
        BaseHeader h;
        if (recv(peer, &h, sizeof(h), 0) == 0) {
            break;
        }

        request.resize(h.bodyLength);
        if (recv(peer, &request[0], h.bodyLength, 0) == 0) {
            break;
        }

        response.clear();
        handleRequest(request, response);

        h.version = 1;
        h.version = response.size();
        if(send(peer, &h, sizeof(h), 0) == 0) {
            break;
        }

        if(send(peer, &response[0], response.size(), 0) == 0) {
            break;
        }
    }

    delete this;
}

StreamServer::StreamServer(unsigned short port, HandlerFactory createHandler) : sock(-1), createHandler(createHandler) {
    char portStr[6];
    sprintf(portStr, "%d", port);

    int err;

    struct addrinfo req = {0};
    req.ai_family = AF_INET;
    req.ai_socktype = SOCK_STREAM;
    req.ai_flags = AI_PASSIVE;

    struct addrinfo *addrList;
    if ((err = getaddrinfo(NULL, portStr, &req, &addrList)) != 0) {
        throw std::runtime_error(gai_strerror(err));
    }

    for (struct addrinfo *addr = addrList; addr; addr = addr->ai_next) {
        if ((sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1) {
            continue;
        }

        if (bind(sock, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }

        close(sock);
        sock = -1;
    }

    freeaddrinfo(addrList);

    if (sock == -1) {
        throw std::runtime_error("Failed to bind");
    }
}

StreamServer::~StreamServer() {
    if (sock != -1) {
        offline();
    }
}

void StreamServer::online(void) {
    assert(sock != -1);
    assert(!listener.joinable());

    listener = std::thread([this]() {
        while (true) {
            if (listen(sock, listenBacklog) == -1) {
                perror("listen");
                break;
            }

            struct sockaddr peerAddr;
            socklen_t peerAddrLen = sizeof(peerAddr);
            int peer = accept(sock, &peerAddr, &peerAddrLen);

            if (peer == -1) {
                perror("accept");
                continue;
            }

            createHandler(peer);
        }
    });
}

void StreamServer::offline(void) {
    assert(sock != -1);
    
    close(sock);
    sock = -1;

    if (listener.joinable()) {
        listener.join();
    }
}
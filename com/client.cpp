#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include <stdexcept>
#include "protocol.hpp"
#include "client.hpp"

std::string echo(const std::string &node, const std::string &service, const std::string &msg) {
    int sock = -1, err;

    struct addrinfo req = {0};
    req.ai_family = AF_INET;
    req.ai_socktype = SOCK_STREAM;

    struct addrinfo *addrList;
    if ((err = getaddrinfo(node.c_str(), service.c_str(), &req, &addrList)) != 0) {
        throw std::runtime_error(gai_strerror(err));
    }

    for (struct addrinfo *addr = addrList; addr; addr = addr->ai_next) {
        if ((sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) == -1) {
            continue;
        }

        if (connect(sock, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }

        close(sock);
        sock = -1;
    }

    freeaddrinfo(addrList);

    if (sock == -1) {
        throw std::runtime_error("Failed to bind");
    }

    try {
        BaseHeader h;
        h.version = 1;
        h.bodyLength = msg.length();

        if (send(sock, &h, sizeof(h), 0) == -1) {
            throw std::runtime_error(strerror(errno));
        }

        if (send(sock, msg.c_str(), h.bodyLength, 0) == -1) {
            throw std::runtime_error(strerror(errno));
        }

        if (recv(sock, &h, sizeof(h), 0) == 0) {
            throw std::runtime_error("Peer dropped when receiving response header");
        }

        std::vector<char> response;
        response.resize(h.bodyLength);
        if (recv(sock, &response[0], h.bodyLength, 0) == 0) {
            throw std::runtime_error("Peer dropped when receiving response body");
        }
        response.push_back('\0');
        return &response[0];
    } catch (std::exception &e) {
        close(sock);
        throw e;
    }
}
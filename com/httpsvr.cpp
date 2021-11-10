#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdio.h>
#include <cstring>
#include <array>
#include <vector>
#include <queue>
#include <map>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <algorithm>
#include <functional>
#include <regex>
#include "httpsvr.hpp"
#include "utils.hpp"
#include "excepts.hpp"
#include "fmt/core.h"

namespace protocol {

static constexpr size_t MAX_RECV_ONCE = 65536;
static const regex HEADER_PATTERN(" *(.+?) *: *(.*?) *\r\n");
static const array<char, 2> CR {'\r', '\n'};
static const array<char, 4> CRCR {'\r', '\n', '\r', '\n'};

static string makeControllerName(const string &method, const string &url);
static args_t makeControllerArgs(const string &method, const string &url);

http::~http() {
    close(peer);
    fmt::print("http session {} exited.\n", (void*) this);
}

msgbuff_t::const_iterator http::receive_header(msgbuff_t &request) const {
    request.clear();
    msgbuff_t::const_iterator headerEnd = request.end();

    while (request.end() == headerEnd /* TODO over-input prevention*/) {
        array<char, MAX_RECV_ONCE> recvBuff;
        auto recvBytes = recv(peer, recvBuff.data(), recvBuff.size(), 0);
        if (0 == recvBytes) {
            throw peer_completion();
        } else if (recvBytes < 0) {
            throw handle_request_failure(500, fmt::format("Server internal error. recv() failed with {} when receiving request header.", errno));
        }

        size_t appendPos = request.size();
        auto insertionPoint = request.insert(request.end(), recvBuff.begin(), recvBuff.begin() + recvBytes);

        headerEnd = search(insertionPoint, request.end(), CRCR.begin(), CRCR.end());
    }

    return headerEnd += 4; // include the "\r\n\r\n" in the header
}

header_t http::parse_header(const msgbuff_t &request, msgbuff_t::const_iterator headerEnd) const {

    header_t header;

    auto firstLineEnd = search(request.begin(), request.end(), CR.begin(), CR.end());
    header[""] = string(request.begin().base(), firstLineEnd.base());
    firstLineEnd += 2;

    auto matchBegin = cregex_iterator(firstLineEnd.base(), headerEnd.base(), HEADER_PATTERN);
    auto matchEnd   = cregex_iterator();
    for (auto i = matchBegin; i != matchEnd; i++) {
        header[(*i)[1]] = (*i)[2];
    }

    return header;
}

tuple<string, string, string> http::parse_start_line(const header_t& header) const {
    string method, url, version;
    stringstream(header.at("")) >> method >> url >> version;
    return {method, url, version};
}

void http::process_header(const header_t &header) {
    try {
        auto &connection = header.at("connection");
        if (connection == "keep-alive") {
            keepAlive = true;
        } else if (connection == "close") {
            keepAlive = false;
        } else {
            throw handle_request_failure(400, fmt::format("Bad request due to unknown connection status '{}'.", connection));
        }
    } catch (...) {
        keepAlive = true;
    }
    
    try {
        contentLength = stoul(header.at("Content-Length"));
    } catch (...) {
        contentLength = 0;
    }
}

tuple<string, args_t> http::controller_name_args(const string &method, const string &url) const {
    return {
        makeControllerName(method, url), 
        makeControllerArgs(method, url)
    };
}

void http::receive_body(msgbuff_t &request) {
    while(request.size() < contentLength) { // contentLength is set by processHeader()
        array<char, MAX_RECV_ONCE> recvBuff;
        int recvBytes = recv(peer, recvBuff.data(), recvBuff.size(), 0);
        if (0 == recvBytes) {
            throw peer_completion();
        } else if (recvBytes < 0) {
            throw handle_request_failure(500, fmt::format("Server internal error. recv() failed with {} when receiving request body.", errno));
        }
        request.insert(request.end(), recvBuff.begin(), recvBuff.begin() + recvBytes);
    }
}

string http::format_header(const header_t &header, const string &statusMsg) const {
    stringstream ss;

    ss << "HTTP/1.1 ";
    if (statusMsg.empty()) {
        ss << "200 OK";
    } else {
        ss << statusMsg;
    }
    ss << "\r\n";

    for (auto item : header) {
        ss << item.first << ':' << item.second << "\r\n";
    }
    ss << "\r\n";

    return ss.str();
}

void http::send_response(const string &header) const {
    if(send(peer, header.c_str(), header.length(), 0) == 0) {
        // TODO handle error
    }
}

void http::send_response(const string &header, const msgbuff_t &body) const {
    send_response(header);

    if(send(peer, &body[0], body.size(), 0) == 0) {
        // TODO handle error
    }
}

void http::start(int peerFd, const string &sessionName) {
    peer = peerFd;
    name = sessionName;
    fmt::print("Client {} was accepted by session {}.\n", sessionName, (void*) this);
    thread([this]() {

        msgbuff_t request;
        header_t  requestHeader;
        msgbuff_t response;
        header_t  responseHeader;
        string    method, url, version;
        string    serviceName;
        args_t    serviceArgs;
    
        try {
            keepAlive = true;
            while (keepAlive) {
                auto headerEndPos = receive_header(request);
                requestHeader = parse_header(request, headerEndPos);
                request.erase(request.begin(), headerEndPos);
                tie(method, url, version) = parse_start_line(requestHeader);
                tie(serviceName, serviceArgs) = controller_name_args(method, url);
                auto controller = find_controller(method, url);
                process_header(requestHeader); // will extract contentLength and keepAlive
                receive_body(request); // rely on contentLength
                tie(responseHeader, response) = controller(request, serviceArgs, requestHeader);
                responseHeader["Content-Length"] = fmt::format("{}", response.size());
                send_response(format_header(responseHeader), response);
            }
        } catch (handle_request_failure &e) {
            responseHeader["Content-Length"] = "0";
            send_response(format_header(responseHeader, e.what()));
        } catch (peer_completion&) {

        } catch (...) {
            responseHeader["Content-Length"] = "0";
            send_response(format_header(responseHeader, "500 Unexpected server internal error."));
        }

        delete this;

    }).detach();
}

application& http::factory(void) {
    return *(new http);
}

http::controller_registry& http::get_controller_registry(void) {
    static controller_registry controllerRegistry;
    return controllerRegistry;
}

void http::register_controller(const string &method, const string &path, controller controllerLambda) {
    get_controller_registry()[makeControllerName(method, path)] = controllerLambda;
}

http::controller http::find_controller(const string &method, const string &path) {
    try {
        return get_controller_registry().at(makeControllerName(method, path));
    } catch (out_of_range &e) {
        throw handle_request_failure(404, fmt::format("No service available at '{}' for '{}'.", path, method));
    }
}

static string makeControllerName(const string &method, const string &url) {
    static const regex URLPATH_PATTERN("(?:http[s]?://)?[^/]*(/[^?]+)");
    smatch m;
    return fmt::format("{}({})", method, regex_search(url, m, URLPATH_PATTERN) ? m[1].str() : "/");
}

static args_t makeControllerArgs(const string &method, const string &url) {
    args_t args;
    for (auto &kv : split(cut(url, '?').second, '&')) {
        args.insert(cut(kv, '='));
    }
    return args;
}

}
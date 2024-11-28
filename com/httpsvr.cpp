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
#include "httpapi.hpp"
#include "utils.hpp"
#include "excepts.hpp"
#include "fmt/core.h"

namespace http {
using namespace server_utils;
using namespace server_excepts;

static constexpr size_t MAX_RECV_ONCE = 65536;
static const regex HEADER_PATTERN(" *(.+?) *: *(.*?) *\r\n");
static const array<char, 2> CR {'\r', '\n'};
static const array<char, 4> CRCR {'\r', '\n', '\r', '\n'};

/**
 * makes the name of a controller by the http method and the url-path
 * @param method the http method
 * @param url the url
 * @return the controller name
 * @note the `url` can be only a path (i.e., "/path/to/the/api") or a full url.
 * the path part will be extracted from a valid url.
 */
static string make_controller_name(const string &url) {
    static const regex URLPATH_PATTERN("(?:http[s]?://)?[^/]*/([^?]+)");
    smatch m;
    return regex_search(url, m, URLPATH_PATTERN) ? m[1].str() : "";
}

/**
 * extracts the arguments (k-v pairs) from a valid url. it is the part after
 * the question mark.
 * @param url the url
 * @return the k-v map representation the arguments
 */
static args_t make_controller_args(const string &url) {
    return args_to_map(cut(url, '?').second, '&', '=');
}

/**
 * receives the header part of an http request. pulls out data from the 
 * beginning till the end-of-header is found. the data is stored in `request` 
 * and the end-pos of the header is returned. a small part of the body 
 * might also be received.
 * @param peer socket fd
 * @param request receiving data buffer
 * @return iterator of the data buffer that points to the next bytes of the
 * end-of-header sign.
 */
static msgbuff_t::const_iterator receive_header(channel *channel, msgbuff_t &request) {
    request.clear();
    msgbuff_t::const_iterator header_end = request.end();

    while (request.end() == header_end /* TODO over-input prevention*/) {
        array<char, MAX_RECV_ONCE> recv_buff;
        auto recv_bytes = channel->recv(recv_buff.data(), recv_buff.size());
        if (0 == recv_bytes) {
            throw peer_completion();
        } else if (recv_bytes < 0) {
            int e = errno;
            if (e == EWOULDBLOCK|| e == EAGAIN)
                throw io_timeout();
            else
                throw handle_request_failure(500, fmt::format("Server internal error. recv() failed with {} when receiving request header.", errno));
        }

        size_t append_pos = request.size();
        auto insertion_point = request.insert(request.end(), recv_buff.begin(), recv_buff.begin() + recv_bytes);

        header_end = search(insertion_point, request.end(), CRCR.begin(), CRCR.end());
    }

    return header_end += CRCR.size(); // include the "\r\n\r\n" in the header
}

/**
 * reads and parses the http header and reorganizes the information in a k-v map.
 * @param request receiving data buffer
 * @param header_end the returned value of receive_header
 * @return the k-v map representation of the http header
 * @note the http start-line will also be in the kv-map with an empty key.
 */
static header_t parse_header(const msgbuff_t &request, msgbuff_t::const_iterator header_end) {

    header_t header;

    auto first_line_end = search(request.begin(), request.end(), CR.begin(), CR.end());
    header[""] = string(request.begin().base(), first_line_end.base());
    first_line_end += CR.size();

    auto match_begin = cregex_iterator(first_line_end.base(), header_end.base(), HEADER_PATTERN);
    auto match_end   = cregex_iterator();
    for (auto i = match_begin; i != match_end; i++) {
        header[(*i)[1]] = (*i)[2];
    }

    return header;
}

/**
 * splits the start-line of the http request into the 3 parts: method, 
 * path, and http version.
 * @param header the returned value of parse_header
 * @return a tuple containing the method, path, and http version.
 */
static tuple<string, string, string> parse_start_line(const header_t& header) {
    string method, url, version;
    stringstream(header.at("")) >> method >> url >> version;
    return {method, url, version};
}

/**
 * extracts information from the http header that the http protocol cares about.
 * @param header the returned value of parse_header
 * @return a tuple containing the keep-alive and Content-Length
 */
static tuple<bool, int> process_header(const header_t &header) {
    bool keep_alive;
    int content_length;
    try {
        auto &connection = header.at("connection");
        if (connection == "keep-alive") {
            keep_alive = true;
        } else if (connection == "close") {
            keep_alive = false;
        } else {
            throw handle_request_failure(400, fmt::format("Bad request due to unknown connection status '{}'.", connection));
        }
    } catch (...) {
        keep_alive = true; // TODO this should be false for HTTP/1.0, but it's no
                           // big deal as the 1.0 clients should close the connection
    }
    
    try {
        content_length = stoul(header.at("Content-Length"));
    } catch (...) {
        content_length = 0;
    }

    return {keep_alive, content_length};
}

/**
 * accepts url and creates the name and arguments for the controller
 * @param url the returned value of parse_start_line
 * @return a tuple containing the name and arguments for the controller
 */
static tuple<string, args_t> controller_name_args(const string &url) {
    return {
        make_controller_name(url), 
        make_controller_args(url)
    };
}

/**
 * it is a continuation of receive_header receiving the rest of the http reuest.
 * @param channel the network channel for communication
 * @param content_length the returned value of process_header
 * @param request the message buffer to reveive the data
 */
static void receive_body(channel *channel, int content_length, msgbuff_t &request) {
    while(request.size() < content_length) { // contentLength is set by processHeader()
        array<char, MAX_RECV_ONCE> recv_buff;
        int recv_bytes = channel->recv(recv_buff.data(), recv_buff.size());
        if (0 == recv_bytes) {
            throw peer_completion();
        } else if (recv_bytes < 0) {
            int e = errno;
            if (e == EWOULDBLOCK|| e == EAGAIN)
                throw io_timeout();
            else
                throw handle_request_failure(500, fmt::format("Server internal error. recv() failed with {} when receiving request header.", errno));
        }
        request.insert(request.end(), recv_buff.begin(), recv_buff.begin() + recv_bytes);
    }
}

/**
 * takes a k-v map form of header and a status string, and creates a reponse 
 * string (the header part) that complies with the http protocol.
 * @param header the k-v map form of header
 * @param status the http status string (i.e., "200 OK")
 * @note if the status has content, it must start with the triple digit http
 * status code. an optional status message can follow the status code separated
 * with a whitespace. it the status is empty, "200 OK" will be used by default.
 */
static string format_header(const header_t &header, const string &status = "") {
    stringstream ss;

    ss << "HTTP/1.1 ";
    if (status.empty()) {
        ss << "200 OK";
    } else {
        ss << status;
    }
    ss << "\r\n";

    for (auto item : header) {
        ss << item.first << ':' << item.second << "\r\n";
    }
    ss << "\r\n";

    return ss.str();
}

/**
 * sends response data
 * @param channel the network channel for communication
 * @param data a random-access container having the data to be sent
 */
template<typename T>
static void send_response(channel *channel, const T &data) {
    if(channel->send(&data[0], data.size()) == 0) {
        // TODO handle error
    }
}

/**
 * a tool class for controller selection. given a method and a url-path,
 * a controller function will be selected to handle the request and roduce 
 * the response.
 */
using controller_registry = api::apis<controller>;

/**
 * gets the instance (singleton) of the controller selection map.
 * @return the controller selection map
 */
static controller_registry& get_controller_registry(void) {
    static controller_registry controller_registry;
    return controller_registry;
}

/**
 * [API] application system uses this API to add the request handling functions.
 * @param method the http method: "GET", "POST", "PUT", "DELETE", ...
 * @param path the url-path the handling function will be associated to
 */
void register_controller(const string &method, const string &path, controller controller_lambda) {
    get_controller_registry().add(path, method, controller_lambda);
}

/**
 * finds the request handling function by http method and url-path.
 * @param method the http method: "GET", "POST", "PUT", "DELETE", ...
 * @param path the url-path the handling function will be associated to
 * @return the request handling function
 */
static tuple<controller, args_t> find_controller(const string &method, const string &path) {
    try {
        return get_controller_registry().find(path, method);
    } catch (out_of_range &e) {
        throw handle_request_failure(404, fmt::format("No service available at '{}' for '{}'.", path, method));
    }
}

/**
 * [API] the http protocol handling function. the application system connects this API 
 * to an initialized tcp server object. when a request if received from a client, the 
 * tcp server picks up a thread in the pool and runs this function. One execution only
 * handles request.
 */
void handler(channel *channel, const string &session_name) {
    // TODO debug_log -> Client {session_name} was accepted by thread {pthread_self()}.

    msgbuff_t request;
    header_t  request_header;
    msgbuff_t response;
    header_t  response_header;
    string    method, url, version;
    string    query_name;
    args_t    query_args;
    bool      keep_alive;
    int       content_length;
    int       peer;

    try {
        string keepAliveHeader = fmt::format("timeout={}", channel->get_info().idol_timeout);
        keep_alive = true;
            
        // receive and parse header
        auto header_end_pos = receive_header(channel, request);
        request_header = parse_header(request, header_end_pos);
        request.erase(request.begin(), header_end_pos); // not quite necessary...TODO pass the request handler the begin/end iterators instead of the whole message buffer

        // handle the start line of the request
        tie(method, url, version) = parse_start_line(request_header);
        tie(query_name, query_args) = controller_name_args(url);

        // find the controller function
        auto controller = find_controller(method, query_name);

        // TODO authorisation should be done before receiving the whole 
        // request body. we can save some time if the client is unauthorised

        // receive request body
        tie(keep_alive, content_length) = process_header(request_header);
        receive_body(channel, content_length, request);

        // call the application system to handle the request
        tie(response_header, response) = get<0>(controller)(request, get<1>(controller), query_args, request_header);

        // send back response
        response_header["Content-Length"] = fmt::format("{}", response.size());
        response_header["Access-Control-Allow-Origin"] = "*";
        if (keep_alive) response_header["Keep-Alive"] = keepAliveHeader;
        send_response(channel, format_header(response_header));
        send_response(channel, response);

    } catch (handle_request_failure &e) {
        response_header["Content-Length"] = "0";
        send_response(channel, format_header(response_header, e.what()));
    } catch (redirect_exception &e) {
        response_header["Content-Length"] = "0";
        response_header["Location"] = e.get_url();
        send_response(channel, format_header(response_header, e.get_msg()));
    } catch (peer_completion &e) {
        throw e;
    } catch (io_timeout &e) {
        throw e;
    } catch (...) {
        response_header["Content-Length"] = "0";
        send_response(channel, format_header(response_header, "500 Unexpected server internal error."));
    }
}

}
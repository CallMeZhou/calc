#pragma once

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <thread>
#include <functional>
#include "protocol.hpp"

namespace protocol {

using namespace std;

class http : public application {
protected:
    string name;
    bool keepAlive;
    int contentLength;
    int peer;

    ~http();

    msgbuff_t::const_iterator receive_header(msgbuff_t &request) const;
    header_t parse_header(const msgbuff_t &request, msgbuff_t::const_iterator headerEnd) const;
    tuple<string, string, string> parse_start_line(const header_t& header) const;
    void process_header(const header_t &header);
    tuple<string, args_t> controller_name_args(const string &method, const string &url) const;
    void receive_body(msgbuff_t &request);
    string format_header(const header_t &header, const string &statusMsg = "") const;
    void send_response(const string &header) const;
    void send_response(const string &header, const msgbuff_t &body) const;

public:
    void start(int peerFd, const string &sessionName) override;
    static application& factory(void);

    using controller = function<tuple<header_t, msgbuff_t>(const msgbuff_t &request, const args_t &args_url, const args_t &args_header)>;
    using controller_registry = map<string, controller>;
    static controller_registry& get_controller_registry(void);
    static void register_controller(const string &method, const string &path, controller controllerLambda);
    static controller find_controller(const string &method, const string &path);
};

#define REGISTER_CONTROLLER_CONCAT2(x, y) x##y
#define REGISTER_CONTROLLER_CONCAT(x, y) REGISTER_CONTROLLER_CONCAT2(x, y)
#define REGISTER_CONTROLLER(method, path, controller_lambda) static auto_executor<decltype(http::register_controller), const char*, const char*, http::controller> REGISTER_CONTROLLER_CONCAT(controller_lambda, __COUNTER__)(http::register_controller, method, path, controller_lambda)

}
#include <iostream>
#include <locale>
#include "fmt/core.h"
#include "libcom.hpp"
#include "utils.hpp"

using namespace std;
using namespace protocol;

tuple<header_t, msgbuff_t> lambda_echo(const msgbuff_t &request, const args_t &args_url, const args_t &args_header) {
    string output = "ECHO: ";

    args_t::const_iterator i;

    if (request.size() > 0) {
        output.append(request.begin().base(), request.end().base());
    } else if (args_url.end() != (i = args_url.find("msg"))) {
        output.append(i->second.begin().base(), i->second.end().base());
    } else if (args_header.end() != (i = args_header.find("msg"))) {
        output.append(i->second.begin().base(), i->second.end().base());
    } else {
        throw handle_request_failure(400, "Nothing to echo.");
    }
    
    return {
        {
            {"Content-Type", "text/html; charset=UTF-8"}
        },

        msgbuff_t(output.begin(), output.end())
    };
}

REGISTER_CONTROLLER("GET", "/echo", lambda_echo);

tuple<header_t, msgbuff_t> lambda_dump(const msgbuff_t &request, const args_t &args_url, const args_t &args_header) {
    fmt::print("[URL arguments]\n");
    for (auto &kv : args_url) {
        fmt::print("{}: {}\n", kv.first, kv.second);
    }
    fmt::print("-------------------------\n");
    fmt::print("[Header]\n");
    for (auto &kv : args_header) {
        fmt::print("{}: {}\n", kv.first, kv.second);
    }
    fmt::print("-------------------------\n");
    fmt::print("[Body]\n");
    for (auto c : request) {
        cout << c;
    }
    cout << endl;
    
    return {{}, msgbuff_t()};
}

REGISTER_CONTROLLER("GET", "/dump", lambda_dump);
REGISTER_CONTROLLER("POST", "/dump", lambda_dump);
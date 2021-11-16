#include <iostream>
#include "fmt/core.h"
#include "libcom.hpp"
#include "utils.hpp"

using namespace std;
using namespace http;

tuple<header_t, msgbuff_t> lambda_dump(const msgbuff_t &request, const args_t &args_url, const args_t &args_query, const args_t &args_header) {
    fmt::print("[URL arguments]\n");
    for (auto &kv : args_query) {
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
    
    return {header_t(), msgbuff_t()};
}

REGISTER_CONTROLLER("GET",    "dump", lambda_dump);
REGISTER_CONTROLLER("POST",   "dump", lambda_dump);
REGISTER_CONTROLLER("PUT",    "dump", lambda_dump);
REGISTER_CONTROLLER("DELETE", "dump", lambda_dump);

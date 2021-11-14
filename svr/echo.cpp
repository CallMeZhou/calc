#include "fmt/core.h"
#include "libcom.hpp"

using namespace std;
using namespace http;
using namespace server_excepts;

tuple<header_t, msgbuff_t> lambda_echo(const msgbuff_t &request, const args_t &args_url, const args_t &args_query, const args_t &args_header) {
    string output = "ECHO: ";

    args_t::const_iterator i;

    if (request.size() > 0) {
        output.append(request.begin().base(), request.end().base());
    } else if (args_query.end() != (i = args_query.find("msg"))) {
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

REGISTER_CONTROLLER("GET",  "echo", lambda_echo);
REGISTER_CONTROLLER("POST", "echo", lambda_echo);
#include <string>
#include <fstream>
#include "spages.hpp"
#include "excepts.hpp"

namespace http {
using namespace server_excepts;

tuple<header_t, msgbuff_t> lambda_static_pages(const msgbuff_t &request, const args_t &args_url, const args_t &args_query, const args_t &args_header) {
    try {
        auto requested_types = split(find_arg(args_header, "Accept", "text/plain"), ',', ' ').at(0);

        auto mode = ios_base::in;
        if (not starts_with(requested_types, "text")) {
            mode |= ios_base::binary;
        }

        ifstream infile(args_url.at("trailer"), mode);
        msgbuff_t repsonse_data((istreambuf_iterator<char>(infile)), istreambuf_iterator<char>());

        header_t response_header {{"Content-Type", requested_types}};

        return {response_header, repsonse_data};
    } catch (...) {
        throw handle_request_failure(404, "file not found");
    }
}

}
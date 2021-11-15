#pragma once
#include <tuple>
#include "httptypes.hpp"

namespace http {

tuple<header_t, msgbuff_t> lambda_static_pages(const msgbuff_t &request, const args_t &args_url, const args_t &args_query, const args_t &args_header);

}
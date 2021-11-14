#pragma once
#include <vector>
#include <map>
#include <functional>
#include "utils.hpp"

namespace http {
using namespace std;
using namespace server_utils;

using msgbuff_t  = vector<char>;
using header_t   = map<string, string>;
using args_t     = header_t;
using controller = function<tuple<header_t, msgbuff_t>(const msgbuff_t &request, const args_t &args_url, const args_t &args_query, const args_t &args_header)>;

}
#pragma once
#include <string>
#include <tuple>
#include "httptypes.hpp"

namespace http {
using namespace std;

class static_pages {
    string workdir;
public:
    static_pages(const string &workdir);
    void operator= (const string &workdir);
    tuple<header_t, msgbuff_t> operator() (const msgbuff_t &request, const args_t &args_url, const args_t &args_query, const args_t &args_header);
};

}

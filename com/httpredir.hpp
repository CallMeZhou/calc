#pragma once
#include <string>
#include <tuple>
#include "httptypes.hpp"

namespace http {
using namespace std;

class redirect {
    string target_dir;
public:
    redirect(const string &target_dir);
    void operator= (const string &target_dir);
    tuple<header_t, msgbuff_t> operator() (const msgbuff_t &request, const args_t &args_url, const args_t &args_query, const args_t &args_header);
};

}

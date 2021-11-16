#include "httpredir.hpp"
#include "httptypes.hpp"
#include "utils.hpp"
#include "excepts.hpp"

namespace http {
using namespace server_excepts;
using namespace server_utils;

redirect::redirect(const string &target_dir) {
    *this = target_dir;
}

void redirect::operator= (const string &target_dir) {
    this->target_dir = target_dir;
    if (not starts_with(this->target_dir, "http") && '/' != this->target_dir[0]) {
        this->target_dir.insert(this->target_dir.begin(), '/');
    }
}

tuple<header_t, msgbuff_t> redirect::operator() (const msgbuff_t &request, const args_t &args_url, const args_t &args_query, const args_t &args_header) {
    throw redirect_exception(308, "Permanent Redirect", "/webgui/index.html");
    return {{},{}};
}

}
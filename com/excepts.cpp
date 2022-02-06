#include "excepts.hpp"
#include "fmt/core.h"

namespace server_excepts {

handle_request_failure::handle_request_failure(int httpStatus, const std::string &errMsg)
: std::runtime_error("") {
    msg = errMsg.empty() ? fmt::format("{}", httpStatus) : fmt::format("{} {}", httpStatus, errMsg);
}

const char* handle_request_failure::what() const noexcept {
    return msg.c_str();
}

redirect_exception::redirect_exception(int httpStatus, const std::string &msg, const std::string &new_url) 
: std::runtime_error(""), new_url(new_url) {
    this->msg = msg.empty() ? fmt::format("{}", httpStatus) : fmt::format("{} {}", httpStatus, msg);
}

const char* redirect_exception::get_msg(void) const {
    return msg.c_str();
}

const char* redirect_exception::get_url(void) const {
    return new_url.c_str();
}

errno_exception::errno_exception(const std::string &errMsg, int err_num) : runtime_error("") {
    msg = fmt::format("{}\nError details: {}", errMsg, strerror(err_num));
}

const char* errno_exception::what() const noexcept {
    return msg.c_str();
}


}
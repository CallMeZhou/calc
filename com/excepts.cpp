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

}
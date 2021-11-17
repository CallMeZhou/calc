#pragma once
#include <string>
#include <stdexcept>

namespace server_excepts {

class peer_completion : public std::runtime_error {
public:
    peer_completion(void) : std::runtime_error("") {}
};

class session_timeout : public std::runtime_error {
public:
    session_timeout(void) : std::runtime_error("") {}
};

class handle_request_failure : public std::runtime_error {
    std::string msg;
public:
    handle_request_failure(int httpStatus, const std::string &errMsg = "");
    const char* what() const noexcept override;
};

class redirect_exception : public std::runtime_error {
    std::string msg;
    std::string new_url;
public:
    redirect_exception(int httpStatus, const std::string &msg, const std::string &new_url);
    const char* get_msg(void) const;
    const char* get_url(void) const;
};

}
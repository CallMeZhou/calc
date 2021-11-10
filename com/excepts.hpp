#pragma once
#include <string>
#include <stdexcept>

class peer_completion : public std::runtime_error {
public:
    peer_completion(void) : std::runtime_error("") {}
};

class handle_request_failure : public std::runtime_error {
    std::string msg;
public:
    handle_request_failure(int httpStatus, const std::string &errMsg = "");
    const char* what() const noexcept override;
};
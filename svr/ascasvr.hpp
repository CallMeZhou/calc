#pragma once
#include "libcom.hpp"

struct ServiceAsciiart : SimpleHandler {
    ServiceAsciiart(int peer);
    ReturnCode handleRequest(const std::vector<uint8_t> &request, BaseHeader *header, std::vector<uint8_t> &response) const override;
    ~ServiceAsciiart();
    static StreamServerHandler* factory(int peer);
};
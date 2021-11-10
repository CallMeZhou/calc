#pragma once
#include "libcom.hpp"
/*
struct ServiceAsciiart : HttpProtocolHandler {
    int requestedWidth;
    using HttpProtocolHandler::HttpProtocolHandler;
    void processHeader(const HeaderType &header) override;
    std::tuple<HeaderType, MessageBuffer> handleRequest(const MessageBuffer &request) const override;
    ~ServiceAsciiart();
    static HttpProtocolHandler* factory(int peer);
};
*/
#pragma once
#include <string>
#include <vector>
#include <thread>
#include <functional>
#include "protocol.hpp"

class StreamServerHandler {
protected:
    int peer;
    virtual void handlerFunc(void) const = 0;

public:
    StreamServerHandler(int peer);
    void start(void);
    virtual ~StreamServerHandler();
};

class SimpleHandler : public StreamServerHandler {
    std::vector<uint8_t> headerBuff;
    void handlerFunc(void) const override;

protected:
    SimpleHandler(int headerBytes, int peer);
    virtual ReturnCode handleRequest(const std::vector<uint8_t> &request, BaseHeader *header, std::vector<uint8_t> &response) const = 0;
};

class StreamServer {
public:
    static const int listenBacklog = 50;
    using HandlerFactory = std::function<StreamServerHandler*(int)>;

private:
    int sock;
    bool stopping;
    std::thread listener;
    HandlerFactory createHandler;

public:
    StreamServer(const std::string &serivce, HandlerFactory createHandler);
    ~StreamServer();
    void online(void);
    void offline(void);
};
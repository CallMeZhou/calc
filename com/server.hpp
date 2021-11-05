#pragma once
#include <thread>
#include <functional>

class StreamServerHandler {
protected:
    int peer;
    virtual void handlerFunc(void) const = 0;

public:
    StreamServerHandler(int peer);
    virtual ~StreamServerHandler();
};

class SimpleHandler : public StreamServerHandler {
    void handlerFunc(void) const override;
    using StreamServerHandler::StreamServerHandler;

protected:
    virtual void handleRequest(const std::vector<char> &request, std::vector<char> &response) const = 0;
};

class StreamServer {
public:
    static const int listenBacklog = 50;
    using HandlerFactory = std::function<StreamServerHandler*(int)>;

private:
    int sock;
    std::thread listener;
    HandlerFactory createHandler;

public:
    StreamServer(unsigned short port, HandlerFactory createHandler);
    ~StreamServer();
    void online(void);
    void offline(void);
};
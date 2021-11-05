#pragma once
#include <string>
#include <vector>
#include <thread>
#include <functional>

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
    bool stopping;
    std::thread listener;
    HandlerFactory createHandler;

public:
    StreamServer(const std::string &serivce, HandlerFactory createHandler);
    ~StreamServer();
    void online(void);
    void offline(void);
};
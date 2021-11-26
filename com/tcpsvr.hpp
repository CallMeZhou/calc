#pragma once
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <functional>
#include "channel.hpp"
#include "thrdpool.hpp"

namespace server {
using namespace std;
using namespace server_utils;
using namespace network_channel;

/**
 * The TCP server that listens to connection requests from clients. It accepts
 * connections and launch an application protocol handling threads for each
 * established connection.
 */
class tcp {
public:
    static const int listen_backlog = 50;

private:
    int sock;
    bool stopping;
    thread listener;

    function<channel*(int)> channel_from_fd;

public:
    /**
     * Constructs the TCP server instance and binds the port.
     * @param server the port number string (i.e., "8000") or the nickname of
     * some well-known ports (i.e., "ssh" for port 22, "telnet" for port 23).
     */
    tcp(const string &serivce, function<channel*(int)> channel_factory);
    ~tcp();
    /**
     * Starts the listening thread. It accepts connections and launch an 
     * application protocol handling threads for each established connection.
     * The specified protocol handling function app_protocol will run in its
     * separate thread.
     * @param app_protocol the protocol handling function
     * @param threads an instance of server_utils::thread_pool
     * 0 means the maximum hardware concurrency plus 2.
     */
    void online(function<void(channel*, const string&)> app_protocol, thread_pool &threads);
    /**
     * Stops the listening thread.
     * @note those running protocol handling threads won't be shut down. You
     * can ignore those threads. They will exit when the clients close the
     * connection.
     */
    void offline(void);
};

}
#pragma once
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <functional>

namespace server {

using namespace std;

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

public:
    /**
     * Constructs the TCP server instance and binds the port.
     * @param server the port number string (i.e., "8000") or the nickname of
     * some well-known ports (i.e., "ssh" for port 22, "telnet" for port 23).
     */
    tcp(const string &serivce);
    ~tcp();
    /**
     * Starts the listening thread. It accepts connections and launch an 
     * application protocol handling threads for each established connection.
     * The specified protocol handling function app_protocol will run in its
     * separate thread.
     * @param app_protocol the protocol handling function
     * @param max_concurrency the maximum number of concurrent connections. 
     * 0 means the maximum hardware concurrency plus 2.
     */
    void online(function<void(int, const string &)> app_protocol, int max_concurrency = 0);
    /**
     * Stops the listening thread.
     * @note those running protocol handling threads won't be shut down. You
     * can ignore those threads. They will exit when the clients close the
     * connection.
     */
    void offline(void);
};

}
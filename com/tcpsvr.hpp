#pragma once
#include <sys/epoll.h>
#include <mutex>
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
    static constexpr int listen_backlog   = 50;
    static constexpr int max_epoll_events = 16;

private:
    int ep_fd;
    int listen_sock;
    int quit_event;
    thread listener;

    using connection = struct {
        string name;
        shared_ptr<channel> chann;
    };
    map<int, connection> connection_map;
    mutex connection_map_lock;
    function<channel*(int)> channel_from_fd;

    /**
     * Adds a new peer fd in the connection_map. Two things will be done for 
     * the new peer: 1. wrap the fd in a channel object (by channel_from_fd); 
     * 2. adds the fd in the epoll so that we can watch the incoming requests.
     * @param peer_fd the new peer fd.
     * @param name a unique string name for the connection.
     */
    void add_new_peer(int peer_fd, const string &name);
    /**
     * Finds the connection name and the channel object of the peer fd.
     * @param peer_fd the peer fd.
     * @return a connection object containing the channel object and the name.
     */
    connection& find_peer_by_fd(int peer_fd);
    /**
     * Re-enable the peer fd in the epoll. The idol timer will be reset.
     * After a peer fd is reported by the epoll_wait() and one request on the fd
     * is completely handled, the fd should be re-enabled to the epoll by this 
     * function. After that the fd enters idol status until the next request comes, 
     * so the idol timer is reset for measuring the length of the idol period.
     * @param peer_fd the peer fd.
     */
    void reenable_peer_by_fd(int peer_fd);
    /**
     * The reversed operation to add_new_peer(). It removes a peer fd from the 
     * connection_map and the epoll. The fd will be closed and so the peer 
     * will be disconnected.
     * @param peer_fd the peer fd.
     * @return the name of the connection for the peer fd to be removed.
     */
    string hangup_peer_by_fd(int peer_fd);
    /**
     * Similar to hangup_peer_by_fd() but accepts the iterator of the connection_map.
     * @param conn the iterator of the connection_map.
     * @return 1.The name of the connection for the peer fd to be removed. 2.The iterator
     * next to the one removed. (usful for removing map items in a loop)
     */
    tuple<string, map<int, connection>::iterator> hangup_peer(map<int, connection>::iterator conn);
    /**
     * Scans the connection_map and hang-up all time-out fds.
     */
    void refresh_peers(void);

public:
    /**
     * Constructs the TCP server instance and binds the port.
     * @param serivce the port number string (i.e., "8000") or the nickname of
     * some well-known ports (i.e., "ssh" for port 22, "telnet" for port 23).
     * @param channel_factory the channel object creator.
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
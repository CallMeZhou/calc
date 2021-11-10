#pragma once
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <functional>
#include "protocol.hpp"

namespace server {

using namespace std;
using namespace protocol;

using protocol_factory_t = function<application&(void)>;

class tcp {
public:
    static const int listen_backlog = 50;

private:
    int sock;
    bool stopping;
    thread listener;
    protocol_factory_t create_app;

public:
    tcp(const string &serivce, protocol_factory_t create_app);
    ~tcp();
    void online(void);
    void offline(void);
};

}
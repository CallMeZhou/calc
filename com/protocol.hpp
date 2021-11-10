#pragma once
#include <string>
#include <vector>
#include <map>

namespace protocol {

using namespace std;

using msgbuff_t = vector<char>;
using header_t  = map<string, string>;
using args_t    = header_t;

class application {
public:

    virtual ~application() {};
    virtual void start(int peerFd, const std::string &sessionName) = 0;
};

};
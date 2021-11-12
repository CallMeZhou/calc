#pragma once

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <thread>
#include <functional>
#include "utils.hpp"

namespace http {
using namespace std;
using namespace server_utils;

using msgbuff_t = vector<char>;
using header_t  = map<string, string>;
using args_t    = header_t;

using controller = function<tuple<header_t, msgbuff_t>(const msgbuff_t &request, const args_t &args_url, const args_t &args_header)>;
void register_controller(const string &method, const string &path, controller controllerLambda);

void handler(int peerFd, const string &sessionName);

#define REGISTER_CONTROLLER_CONCAT2(x, y) x##y
#define REGISTER_CONTROLLER_CONCAT(x, y) REGISTER_CONTROLLER_CONCAT2(x, y)
#define REGISTER_CONTROLLER(method, path, controller_lambda) static auto_executor<decltype(register_controller), const char*, const char*, controller> REGISTER_CONTROLLER_CONCAT(controller_lambda, __COUNTER__)(register_controller, method, path, controller_lambda)

}
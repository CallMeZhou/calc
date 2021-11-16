#pragma once

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <thread>
#include <functional>
#include "utils.hpp"
#include "httptypes.hpp"

namespace http {
using namespace std;
using namespace server_utils;

void register_controller(const string &method, const string &path, controller controllerLambda);
void handler(int peerFd, const string &sessionName);

#define REGISTER_CONTROLLER_CONCAT2(x, y) x##y
#define REGISTER_CONTROLLER_CONCAT(x, y) REGISTER_CONTROLLER_CONCAT2(x, y)
#define REGISTER_CONTROLLER(method, path, controller_lambda) static server_utils::auto_executor<decltype(http::register_controller), const char*, const char*, http::controller> REGISTER_CONTROLLER_CONCAT(controller_lambda, __COUNTER__)(http::register_controller, method, path, controller_lambda)

}
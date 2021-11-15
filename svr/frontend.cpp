#include "libcom.hpp"

using namespace http;

REGISTER_CONTROLLER("GET", "client/{trailer:**}", lambda_static_pages);

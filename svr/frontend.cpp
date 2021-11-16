#include "libcom.hpp"
#include "siteconf.hpp"
using namespace http;
using namespace server_excepts;

http::static_pages frontend_pages(add_home_dir(getconf("webgui/workdir", "")));
REGISTER_CONTROLLER("GET", "webgui/{trailer:**}", frontend_pages);

http::redirect redirect_to_frontpage("/webgui/index.html");
REGISTER_CONTROLLER("GET", "webgui",     redirect_to_frontpage);
REGISTER_CONTROLLER("GET", "",           redirect_to_frontpage);
REGISTER_CONTROLLER("GET", "index.html", redirect_to_frontpage);
REGISTER_CONTROLLER("GET", "index.htm",  redirect_to_frontpage);

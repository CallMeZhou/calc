#include <vector>
#include <fstream>
#include <algorithm>
#include <filesystem>
//#include <mutex>
#include "siteconf.hpp"
using namespace std;
using namespace filesystem;
using namespace picojson;

static const char* site_conf_filename = "siteconf.json";

const value& getconf(void) {
    static bool loaded = false;
    static value conf;
//    static mutex lock;

//    lock_guard<mutex> guard(lock);
    if (!loaded) {
        ifstream infile(path(_getenv("CALC_SITE_HOME", "")) / site_conf_filename);
        string site_conf_content((istreambuf_iterator<char>(infile)), istreambuf_iterator<char>());
        picojson::parse(conf, site_conf_content);
        loaded = true;
    }

    return conf;
}

template<>
const char* getconf(const std::string &conf_path, const char *defval) {
    try {
        auto path_nodes = server_utils::split(conf_path, '/', ' ');
        auto conf_node = &(getconf().get<picojson::object>());
        for_each(path_nodes.begin(), path_nodes.end() - 1, [&conf_node](const std::string &n) {
            conf_node = &(conf_node->at(n).get<picojson::object>());
        });
        return conf_node->at(path_nodes.back()).get<std::string>().c_str();
    } catch (...) {
        return defval;
    }
}

std::string& get_site_home(void) {
    static string site_home_dir = _getenv(SITE_HOME_ENVAR, "");
    return site_home_dir;
}

std::string add_home_dir(const std::string &sub_dir) {
    return path(get_site_home()) / sub_dir;
}
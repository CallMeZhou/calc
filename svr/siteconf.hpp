#pragma once
#include <cstdlib>
#include <string>
#include "utils.hpp"
#include "picojson.h"

const picojson::value& getconf(void);

template<typename VALUE_TYPE>
VALUE_TYPE getconf(const std::string &conf_path, VALUE_TYPE defval) {
    try {
        auto path_nodes = server_utils::split(conf_path, '/', ' ');
        auto conf_node = &(getconf().get<picojson::object>());
        for_each(path_nodes.begin(), path_nodes.end() - 1, [&conf_node](const std::string &n) {
            conf_node = &(conf_node->at(n).get<picojson::object>());
        });
        return conf_node->at(path_nodes.back()).get<VALUE_TYPE>();
    } catch (...) {
        return defval;
    }
}

template<> const char* getconf(const std::string &conf_path, const char *defval);

template<typename VALUE_TYPE>
VALUE_TYPE _getenv(const char *name, VALUE_TYPE defval);

template<> inline
std::string _getenv(const char *name, std::string defval) {
    auto env = getenv(name);
    return env ? env : defval;
}

template<> inline
const char* _getenv(const char *name, const char *defval) {
    auto env = getenv(name);
    return env ? env : defval;
}

template<> inline
int _getenv(const char *name, int defval) {
    return atoi(_getenv(name, "0"));
}

template<> inline
double _getenv(const char *name, double defval) {
    return atof(_getenv(name, "0"));
}

constexpr auto SITE_HOME_ENVAR = "CALC_SITE_HOME";
extern std::string site_home_dir;

std::string add_home_dir(const std::string &sub_dir);
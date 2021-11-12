#include <sstream>
#include <algorithm>
#include "utils.hpp"

namespace server_utils{

std::vector<std::string> split(const std::string &s, char delim, char tobetrimmed) {
    std::istringstream ss(s);
    std::vector<std::string> result;
    std::string part;
    if (0 == tobetrimmed) {
        while(std::getline(ss, part, delim)) {
            result.push_back(std::move(trim(part, tobetrimmed)));
        }
    } else {
        while(std::getline(ss, part, delim)) {
            result.push_back(std::move(part));
        }
    }
    return result;
}

std::pair<std::string, std::string> cut(const std::string &s, char delim, char tobetrimmed) {
    using RType = std::pair<std::string, std::string>;
    auto cutPos = s.find_first_of(delim);
    if (0 == tobetrimmed) {
        return cutPos == std::string::npos ? RType(s, "") : RType(s.substr(0, cutPos), s.substr(cutPos + 1));
    } else {
        if (cutPos == std::string::npos) {
            auto left = s;
            return RType(trim(left, tobetrimmed), "");
        } else {
            auto left  = s.substr(0, cutPos);
            auto right = s.substr(cutPos + 1);
            return RType(trim(left, tobetrimmed), trim(right, tobetrimmed));
        }
    }
}

std::map<std::string, std::string> args_to_map(const std::string &args, char arg_sep, char kv_sep) {
    std::map<std::string, std::string> result;
    for (auto &kv : split(args, arg_sep)) {
        result.insert(cut(kv, kv_sep, ' '));
    }
    return result;
}

bool starts_with(const std::string &s, const std::string prefix) {
    return 0 == s.compare(0, prefix.size(), prefix);
}

std::string& trim_left(std::string &s, char tobetrimmed) {
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [tobetrimmed](auto c){
        return c == tobetrimmed;
    }));
    return s;
}

std::string& trim_right(std::string &s, char tobetrimmed) {
    s.erase(std::find_if_not(s.rbegin(), s.rend(), [tobetrimmed](auto c){
        return c == tobetrimmed;
    }).base(), s.end());
    return s;
}

std::string& trim(std::string &s, char tobetrimmed) {
    trim_left(s, tobetrimmed);
    trim_right(s, tobetrimmed);
    return s;
}


}
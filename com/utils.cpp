#include <sstream>
#include "utils.hpp"

std::vector<std::string> split(const std::string s, char delim) {
    std::istringstream ss(s);
    std::vector<std::string> result;
    std::string part;
    while(std::getline(ss, part, delim)) {
        result.push_back(std::move(part));
    }
    return result;
}

std::pair<std::string, std::string> cut(const std::string s, char delim) {
    using RType = std::pair<std::string, std::string>;
    auto cutPos = s.find_first_of(delim);
    return cutPos == std::string::npos ? RType(s, "") : RType(s.substr(0, cutPos), s.substr(cutPos + 1));
}
#pragma once
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <tuple>
#include <iterator>
#include <algorithm>
#include <functional>

namespace server_utils {

/**
 * Splits the given string to a array of strings.
 * @param s the string to split
 * @param delim the character of the splitting point
 * @param tobetrimmed the leading and tailing char to trim. 0 to skip trimming.
 * @return the splitted parts of the input string
 */
std::vector<std::string> split(const std::string &s, char delim, char tobetrimmed = 0);

/**
 * Cuts the given string to 2 halfs. This is similar to split() but only split
 * the input string once at the first delim character.
 * @param s the string to cut
 * @param delim the character of the cutting point
 * @param tobetrimmed the leading and tailing char to trim. 0 to skip trimming.
 * @return the 2 halfs of the input string
 */
std::pair<std::string, std::string> cut(const std::string &s, char delim, char tobetrimmed = 0);

std::map<std::string, std::string> args_to_map(const std::string &args, char arg_sep = ';', char kv_sep = '=');

bool starts_with(const std::string &s, const std::string prefix);

std::string& trim_left(std::string &s, char tobetrimmed = ' ');
std::string& trim_right(std::string &s, char tobetrimmed = ' ');
std::string& trim(std::string &s, char tobetrimmed = ' ');

template<typename _ForwardIterator>
std::tuple<std::string, _ForwardIterator> getline(_ForwardIterator first, _ForwardIterator last, const std::string &line_ending) {
    _ForwardIterator end_pos = std::search(first, last, line_ending.begin(), line_ending.end());
    _ForwardIterator next_pos = end_pos;
    if (next_pos != last) {
        for (size_t i = 0; i < line_ending.size(); i++) {
            next_pos++;
        }
    }
    return { std::string(first, end_pos), next_pos };
}

template<typename M1, typename M2>
std::string find_arg(const M1 &args1, const M2 &args2, const std::string &key, const std::string &def_val) {
    try { return args1.at(key); } catch (...) {}
    try { return args2.at(key); } catch (...) {}
    return def_val;
}

/**
 * A utility class that automatically runs your function when the program starts.
 * 
 * Usage example 1:
 * 
 * void my_func(int arg1, double arg2, const std::string &arg3) {
 *     // ...
 * }
 * 
 * auto_executor<decltype(my_func), int, double, const std::string&> auto_run_my_func(42, 3.14, "start info");
 */
template<typename FUNC, typename... ARGS> struct auto_executor { auto_executor(FUNC func, ARGS ... args) { func(args ...); } };

struct deferred { std::function<void(void)> action; deferred(decltype(action) action) : action(action) {} ~deferred() { if (action) action(); } };

}
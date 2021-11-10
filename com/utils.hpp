#pragma once
#include <string>
#include <vector>
#include <utility>
#include <functional>

/**
 * Splits the given string to a array of strings.
 * @param s the string to split
 * @param delim the character of the splitting point
 * @return the splitted part of the input string
 */
std::vector<std::string> split(const std::string s, char delim);

/**
 * Cuts the given string to 2 halfs. This is similar to split() but only split
 * the input string once at the first delim character.
 */
std::pair<std::string, std::string> cut(const std::string s, char delim);

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

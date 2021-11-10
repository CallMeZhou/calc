#pragma once
#include <string>
#include <vector>
#include <utility>
#include <functional>

std::vector<std::string> split(const std::string s, char delim);
std::pair<std::string, std::string> cut(const std::string s, char delim);
template<typename FUNC, typename... ARGS> struct auto_executor { auto_executor(FUNC func, ARGS ... args) { func(args ...); } };

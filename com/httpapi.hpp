#pragma once
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <stdexcept>
#include <functional>
#include <tuple>
#include <regex>
#include <stdexcept>
#include "utils.hpp"
#include "excepts.hpp"
#include "httptypes.hpp"
#include "fmt/core.h"

namespace http {

namespace api {

using namespace std;
using namespace server_utils;
using namespace server_excepts;

/**
 * the type of a node in a path. a node of a path is the section between two
 * slashes. suppose an url pattern is as below:
 * GET http://website:80/api/{ver:v1\.[2-4]}/user/{id}/profile/* /{trailer:**}
 * the types of nodes are:
 * TEXT     - api, user, profile
 * WILDCARD - {id}, abbr of {id:.*}. matches any string and assigns to variable "id"
 * WILDCARD - {ver:v1\.[2-4]}. matches v1.2, v1.3, v1.4 and assigns to variable "ver"
 * WILDCARD - *, abbr of {:.*}. matches any string without capturing the value
 * TRAILER_WILDCARD - {trailer:**}. must be the last section. matches the rest
 */
enum class path_node_type { TEXT, WILDCARD, TRAILER_WILDCARD };

/**
 * [internal] the node structure of a parsed path
 */
template<typename entrance_t>
struct path_node {
    string         text;
    string         variable;
    regex          pattern;
    path_node_type type;
    vector<size_t> subs;
    entrance_t     entrance;
    path_node (string text, path_node_type type) : text(text), type(type) {
        if (is_wildcard()) {
            auto parts = cut(text, ':');

            if (not starts_with(parts.first, "*"))
                variable = parts.first;

            if (parts.second.empty() || parts.second == "*")
                pattern = regex(".*");
            else if (parts.second == "**")
                pattern = regex(".*");
            else
                pattern = regex(parts.second);
        }
    }
    bool is_wildcard(void) const {
        return type != path_node_type::TEXT;
    }
    string get_diag_text(void) const {
        if (path_node_type::WILDCARD == type) {
            return '{' + text + '}';
        }
        return text;
    }
    bool match(const string &nodestr) const {
        if (type == path_node_type::TRAILER_WILDCARD) return true;
        return is_wildcard() ? regex_match(nodestr, pattern) : (text == nodestr);
    }
    bool match(const path_node &node) const {
        return match(node.text);
    }
};

/**
 * the main interface of the api module.
 */
template<typename entrance_t>
class apis {
    vector<path_node<entrance_t>> api_trie;
    int n_methods;

    int find_method_node(const string &method) {
        for (int i = 0; i < n_methods; i++) {
            if (0 == strcasecmp(api_trie[i].text.c_str(), method.c_str())) return i;
        }

        throw out_of_range(fmt::format("unknown method: '{}'", method));
    }

public:
    apis(void) {
        for (auto method : {"GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE", "PATCH"}) {
            api_trie.emplace_back(method, path_node_type::TEXT);
        }
        n_methods = (int) api_trie.size();
    }

    /**
     * add an api entrance and associate it with a path pattern and a method.
     * the path pattern is consisted of multiple string sections separated by
     * '/', but shouldn't prefixed and suffixed with '/'. the string section 
     * could be valid url characters or be wildcards.
     * 
     * you can use one of the following patterns or a combination of them:
     * 
     * basic form:
     * api/v1/some-service
     * 
     * with regex and capture the value to variable "ver"
     * api/{ver:v[1-9]}/some-service
     * 
     * with regex but ignore the matched value
     * api/{:v[1-9]}/some-service
     * 
     * equals to api/{ver:.*}/some-service
     * api/{ver}/some-service
     * 
     * equals to api/{:.*}/some-service
     * api/* /some-service
     * 
     * @param path the path pattern.
     * @param method the method name.
     */
    void add(const string &path, const string &method, entrance_t entrance) {
        static const regex path_validater(
            "(^[0-9,a-z,A-Z,\\-,.,_,~]+$)|" // m[1] normal text
            "^\\{(.*?)\\}$");               // m[2] path variable

        vector<path_node<entrance_t>> nodes;

        bool trailer_wildcard_found = false;
        for (auto &section : split(path, '/')) {
            if (trailer_wildcard_found) {
                throw runtime_error(fmt::format("'**' pattern must be in the last section: '{}'", path));
            }

            smatch m;
            if (!regex_match(section, m, path_validater)) {
                throw runtime_error(fmt::format("url pattern malformed: '{}'", path));
            }

            if (not m[1].str().empty()) {
                nodes.emplace_back( m[1], path_node_type::TEXT );
            } else if (m[2].str().find("**") != string::npos) {
                nodes.emplace_back( m[2], path_node_type::TRAILER_WILDCARD );
                trailer_wildcard_found = true;
            } else {
                nodes.emplace_back( m[2], path_node_type::WILDCARD );
            }
        }

        int curr_node_index = find_method_node(method);

        for (auto &new_node : nodes) {
            bool node_matched = false;

            for (auto i : api_trie[curr_node_index].subs) {
                auto &curr_node = api_trie[i];
                bool collision = false;

                if (curr_node.type == path_node_type::TRAILER_WILDCARD) {
                    collision = true;
                } else if (curr_node.text == new_node.text) {
                    curr_node_index = i;
                    node_matched = true;
                    break;
                } else if (curr_node.is_wildcard() && not new_node.is_wildcard()) {
                    collision = curr_node.match(new_node);
                } else if (not curr_node.is_wildcard() && new_node.is_wildcard()) {
                    collision = new_node.match(curr_node);
                } // else if (curr_node.isWildcard() || new_node.isWildcard())... 
                // no way to detect collision in this case although it's worth doing :(

                if (collision) throw runtime_error(fmt::format("path nodes collision. existing: '{}', new: '{}' (in '{}')",  curr_node.get_diag_text(), new_node.get_diag_text(), path));
            }

            if (!node_matched) {
                api_trie.push_back(move(new_node));
                auto &parent_node = api_trie[curr_node_index];
                curr_node_index = api_trie.size() - 1;
                parent_node.subs.push_back(curr_node_index);
            }
        }

        auto &insertion_node = api_trie[curr_node_index];
        if (insertion_node.entrance) throw runtime_error(fmt::format("an api already exists at the specified path: '{}'", path));
        insertion_node.entrance = entrance;
    }

    /**
     * get an api entrance function and the path parameters.
     * 
     * suppose an url pattern is as below:
     * GET http://website:80/api/{ver:v1\.[2-4]}/user/{id}/profile/*
     * 
     * and an requesting url is:
     * GET http://website:80/api/v1.3/user/zhou/profile/*
     * 
     * the content of the path parameters map will be:
     * { {"ver", "1.3"}, {"id", "zhou"} }
     * 
     * @param path the path of the requesting url.
     * @param method the method of the requesting url.
     * @return it returns a dual-element tuple. std::get<0> - the entrance 
     * function. std::get<1> - the path parameters.
     */
    tuple<entrance_t, args_t> find(const string &path, const string &method) {
        args_t pparams;
        int curr_node_index = find_method_node(method);
        auto nodes = split(path, '/');
        stringstream trailer;

        for (auto &new_node : nodes) {
            bool node_matched = false;

            if (api_trie[curr_node_index].type == path_node_type::TRAILER_WILDCARD) {
                node_matched = true;
                trailer << new_node << '/';
            } else {
                for (auto i : api_trie[curr_node_index].subs) {
                    auto &curr_node = api_trie[i];

                    if (curr_node.match(new_node)) {
                        curr_node_index = i;
                        node_matched = true;
                        if (curr_node.type == path_node_type::TRAILER_WILDCARD) {
                            trailer << new_node << '/';
                        } else if (not curr_node.variable.empty()) {
                            pparams[curr_node.variable] = new_node;
                        }
                        break;
                    }
                }
            }

            if (!node_matched) throw out_of_range(new_node);
        }

        auto &curr_node = api_trie[curr_node_index];

        if (curr_node.type == path_node_type::TRAILER_WILDCARD && not curr_node.variable.empty()) {
            string trailer_path = trailer.str();
            pparams[curr_node.variable] = trailer_path.substr(0, trailer_path.length() - 1);
        }

        auto entrance = curr_node.entrance;
        if (!entrance) throw out_of_range("no entrance");

        return { entrance, pparams };
    }
};

}}

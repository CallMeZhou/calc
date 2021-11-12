#include <array>
#include <algorithm>
#include <stdexcept>
#include "excepts.hpp"
#include "utils.hpp"
#include "formdata.hpp"
#include "fmt/core.h"

namespace http {
namespace multpart_formdata {

using namespace server_excepts;
using namespace server_utils;

static const array<char, 2> CR {'\r', '\n'};

vector<form_entry> parse(const args_t &args_header, const msgbuff_t &request_body) {
    // expecting: Content-Type: multipart/form-data; boundary=--...
    auto content_type = cut(args_header.at("Content-Type"), ';');
    if (content_type.first != "multipart/form-data") {
        throw handle_request_failure(400, "Not a 'multipart/form-data'");
    }
    
    auto boundary = cut(trim(content_type.second), '=');
    if (boundary.first != "boundary") {
        throw handle_request_failure(400, "'boundary=' not found");
    } else if (boundary.second.empty()) {
        throw handle_request_failure(400, "'boundary' not specified");
    }

    vector<form_entry> result;
    
    string entry_start = "--" + boundary.second;
    auto pos = search(request_body.begin(), request_body.end(), entry_start.begin(), entry_start.end());

    while (request_body.end() != pos) {
        // jump over boundary text and check for end-of-form flag
        pos += entry_start.size();
        if ('-' == *pos) {
            break;
        } else {
            pos += 2;
        }

        // parse entry header
        form_entry entry;
        string line;
        while (true) {
            tie(line, pos) = server_utils::getline(pos, request_body.end(), "\r\n");
            if (line.empty()) {
                break;
            }

            auto kv = cut(line, ':');
            entry.header[kv.first] = args_to_map(kv.second);
        }

        // copy entry body
        auto next = search(pos, request_body.end(), entry_start.begin(), entry_start.end());
        entry.body.assign(pos, next);

        // store and move to the next entry
        result.push_back(move(entry));
        pos = next;
    }

    return result;
}

form_entry& find(vector<form_entry> &data, const string &header_key, const string &arg_key, const string &arg_value) {
    auto quoted_arg_value = fmt::format("\"{}\"", arg_value);
    auto entry = find_if(data.begin(), data.end(), [&](auto &entry) {
        try {
            return quoted_arg_value == entry.header.at(header_key).at(arg_key);
        } catch (out_of_range&) {
            return false;
        }
    });

    if (entry == data.end()) {
        throw out_of_range(fmt::format("'{}' header or '{}' argument not found.", header_key, arg_key));
    }

    return *entry;
}


}}
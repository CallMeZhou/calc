#pragma once
#include <string>
#include <vector>
#include <map>
#include "httpsvr.hpp"

namespace http {
namespace multpart_formdata {

using namespace std;

// i.e., Content-Disposition: form-data; name="..."; filename="..."
using form_entry_header = map<string, args_t>;

struct form_entry {
    form_entry_header header;
    msgbuff_t body;
};

vector<form_entry> parse(const args_t &args_header, const msgbuff_t &request_body);
form_entry& find(vector<form_entry> &data, const string &header_key, const string &arg_key, const string &arg_value);

}}
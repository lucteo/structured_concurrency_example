#pragma once

#include <string>
#include <vector>

namespace http_server {

//! An HTTP header, that can belong both to a request and a response
struct header {
    std::string name_;
    std::string value_;
};

//! A list of headers to be included in the request of in the response
using headers = std::vector<header>;

} // namespace http_server
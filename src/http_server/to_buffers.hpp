#pragma once

#include "http_response.hpp"

#include <vector>
#include <string_view>

namespace http_server {

//! Converts an HTTP response object to a vector of buffers, ready to be sent over a stream
void to_buffers(const http_response& resp, std::vector<std::string_view>& buffers);

} // namespace http_server
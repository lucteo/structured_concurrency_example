#pragma once

#include "http_response.hpp"

#include <string_view>

namespace http_server {

struct http_response;

//! Create a response with no body, just from the status code
http_response create_response(status_code sc);

//! Creates a response with the given body (of the given content type)
http_response create_response(status_code sc, std::string_view content_type, std::string body);

//! Creates a response with the given headers
http_response create_response(status_code sc, headers hs);

//! Creates a response with the given headers and the body (of the given content type)
http_response create_response(
        status_code sc, headers hs, std::string_view content_type, std::string body);

} // namespace http_server
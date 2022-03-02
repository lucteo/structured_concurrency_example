#pragma once

#include "headers.hpp"

#include <string>
#include <vector>

namespace http_server {

//! The status codes that we support for HTTP responses
enum class status_code {
    s_200_ok,
    s_201_created,
    s_202_accepted,
    s_204_no_content,
    s_300_multiple_choices,
    s_301_moved_permanently,
    s_302_moved_temporarily,
    s_304_not_modified,
    s_400_bad_request,
    s_401_unauthorized,
    s_403_forbidden,
    s_404_not_found,
    s_500_internal_server_error,
    s_501_not_implemented,
    s_502_bad_gateway,
    s_503_service_unavailable,
};

//! Structure describing an HTTP response to be sent to the clients.
//! We aim for a simple representation here, not the most efficient one.
struct http_response {
    //! The status code of the response; e.g., 200 OK
    const status_code status_code_;
    //! The headers of the response
    const headers headers_;
    //! The body of the response, if we have one
    const std::string body_;
};
} // namespace http_server

#include "create_response.hpp"
#include "http_response.hpp"

namespace http_server {

http_response create_response(status_code sc) { return http_response{sc, {}, {}}; }

http_response create_response(status_code sc, std::string_view content_type, std::string body) {
    header h{"Content-type", std::string{content_type}};
    headers headers = {std::move(h)};
    return http_response{sc, std::move(headers), std::move(body)};
}

http_response create_response(status_code sc, headers hs) {
    return http_response{sc, std::move(hs), {}};
}

http_response create_response(
        status_code sc, headers hs, std::string_view content_type, std::string body) {
    hs.emplace_back(header{"Content-type", std::string{content_type}});
    return http_response{sc, std::move(hs), std::move(body)};
}

} // namespace http_server

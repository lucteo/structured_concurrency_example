#include "to_buffers.hpp"

namespace http_server {

namespace {

using namespace std::literals;

const std::string_view crlf = "\r\n"sv;
const std::string_view header_separator = ": "sv;

std::string_view status_code_to_string(status_code sc) {
    switch (sc) {
    case status_code::s_200_ok:
        return "HTTP/1.1 200 OK\r\n"sv;
    case status_code::s_201_created:
        return "HTTP/1.1 201 Created\r\n"sv;
    case status_code::s_202_accepted:
        return "HTTP/1.1 202 Accepted\r\n"sv;
    case status_code::s_204_no_content:
        return "HTTP/1.1 204 No Content\r\n"sv;
    case status_code::s_300_multiple_choices:
        return "HTTP/1.1 300 Multiple Choices\r\n"sv;
    case status_code::s_301_moved_permanently:
        return "HTTP/1.1 301 Moved Permanently\r\n"sv;
    case status_code::s_302_moved_temporarily:
        return "HTTP/1.1 302 Moved Temporarily\r\n"sv;
    case status_code::s_304_not_modified:
        return "HTTP/1.1 304 Not Modified\r\n"sv;
    case status_code::s_400_bad_request:
        return "HTTP/1.1 400 Bad Request\r\n"sv;
    case status_code::s_401_unauthorized:
        return "HTTP/1.1 401 Unauthorized\r\n"sv;
    case status_code::s_403_forbidden:
        return "HTTP/1.1 403 Forbidden\r\n"sv;
    case status_code::s_404_not_found:
        return "HTTP/1.1 404 Not Found\r\n"sv;
    case status_code::s_500_internal_server_error:
        return "HTTP/1.1 500 Internal Server Error\r\n"sv;
    case status_code::s_501_not_implemented:
        return "HTTP/1.1 501 Not Implemented\r\n"sv;
    case status_code::s_502_bad_gateway:
        return "HTTP/1.1 502 Bad Gateway\r\n"sv;
    case status_code::s_503_service_unavailable:
        return "HTTP/1.1 503 Service Unavailable\r\n"sv;
    }
}

} // namespace

void to_buffers(const http_response& resp, std::vector<std::string_view>& buffers) {
    buffers.reserve(1 + resp.headers_.size() * 4 + 2);

    buffers.push_back(status_code_to_string(resp.status_code_));
    for (const auto& p : resp.headers_) {
        buffers.push_back(std::string_view(p.name_));
        buffers.push_back(header_separator);
        buffers.push_back(std::string_view(p.value_));
        buffers.push_back(crlf);
    }
    buffers.push_back(crlf);
    if (!resp.body_.empty())
        buffers.push_back(std::string_view(resp.body_));
}

} // namespace http_server
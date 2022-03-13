#include "request_parser.hpp"
#include "profiling.hpp"

#include <algorithm>
#include <cctype>

namespace http_server {

namespace {
std::pair<http_method, bool> parse_method(std::string_view method_str) {
    if (method_str == "GET")
        return {http_method::get, true};
    if (method_str == "HEAD")
        return {http_method::head, true};
    if (method_str == "POST")
        return {http_method::post, true};
    if (method_str == "PUT")
        return {http_method::put, true};
    if (method_str == "DELETE")
        return {http_method::delete_method, true};
    if (method_str == "CONNECT")
        return {http_method::connect, true};
    if (method_str == "OPTIONS")
        return {http_method::options, true};
    if (method_str == "TRACE")
        return {http_method::trace, true};
    if (method_str == "PATCH")
        return {http_method::patch, true};
    return {http_method::get, false};
}
} // namespace

std::optional<http_request> request_parser::parse_next_packet(std::string_view data) {
    PROFILING_SCOPE();
    if (state_ == parse_state::done)
        return {};

    // Everything before the body, we parse line by line
    while (state_ < parse_state::body && !data.empty()) {
        auto eol_pos = data.find("\r\n");
        if (eol_pos == std::string_view::npos) {
            // Partial line
            cur_line_.append(data);
            return {};
        } else {
            // We read a complete line
            cur_line_.append(data.substr(0, eol_pos));
            data.remove_prefix(eol_pos + 2);

            // Interpret this line
            if (!add_current_line())
                throw bad_request{};
            cur_line_.clear();
        }
    }

    // Check if we have new data for the body
    if (state_ == parse_state::body) {
        if (body_remaining_ >= data.size()) {
            body_.append(data);
            body_remaining_ -= data.size();
        } else {
            body_.insert(body_.end(), data.begin(), data.begin() + body_remaining_);
            body_remaining_ = 0;
        }

        // Did we complete?
        if (body_remaining_ == 0) {
            state_ = parse_state::done;
            // return the read HTTP request object
            return {http_request{method_, std::move(uri_), std::move(headers_), std::move(body_)}};
        }
    }
    return {};
}

bool request_parser::add_current_line() {
    std::string_view line{cur_line_};
    if (state_ == parse_state::first_line) {
        // Parse the method
        auto pos = std::min(line.find(' '), line.size());
        auto [method_, ok] = parse_method(line.substr(0, pos));
        if (!ok)
            return false;

        // Parse the URI
        auto pos0 = pos + 1;
        pos = std::min(line.find(" HTTP/", pos0), line.size());
        uri_ = line.substr(pos0, pos - pos0);

        // Move to the next line
        state_ = parse_state::header_lines;
    } else {
        if (line.empty()) {
            // Empty line: end of headers
            state_ = parse_state::body;
        } else {
            // header name
            auto pos = std::min(line.find(':'), line.size());
            auto hname = line.substr(0, pos);
            line.remove_prefix(pos + 1);
            // skip optional whitespace
            line.remove_prefix(std::min(line.find_first_not_of(' '), line.size()));
            // header value
            pos = std::min(line.rfind(' '), line.size());
            auto hval = line.substr(0, pos);
            if (hname.empty() || hval.empty())
                return false;

            // Transform the header name in lowercase
            std::string name{hname};
            std::transform(name.begin(), name.end(), name.begin(), std::tolower);
            // Check for content-length
            std::string val{hval};
            if (name == "content-length") {
                body_remaining_ = static_cast<size_t>(std::stoull(val));
                body_.reserve(body_remaining_);
            }

            // Add the header
            headers_.push_back(header{std::move(name), std::move(val)});
        }
    }
    return true;
}

} // namespace http_server
#pragma once

#include "http_request.hpp"

#include <string>
#include <string_view>
#include <optional>
#include <stdexcept>

namespace http_server {

struct bad_request : std::exception {
    const char* what() const noexcept override { return "bad HTTP request"; }
};

class request_parser {
public:
    std::optional<http_request> parse_next_packet(std::string_view data);

private:
    enum class parse_state {
        first_line,
        header_lines,
        body,
        done,
    };
    parse_state state_{parse_state::first_line};
    std::string cur_line_;
    http_method method_{http_method::get};
    std::string uri_;
    headers headers_;
    size_t body_remaining_{0};
    std::string body_;

    bool add_current_line();
};
} // namespace http_server
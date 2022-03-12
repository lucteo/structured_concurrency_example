#pragma once

#include "http_server/request_parser.hpp"
#include "io/io_context.hpp"
#include "io/connection.hpp"
#include "io/async_read.hpp"

#include <task.hpp>

auto read_http_request(io::io_context& ctx, const io::connection& conn)
        -> task<http_server::http_request> {
    http_server::request_parser parser;
    char buf[1024];
    io::out_buffer out_buf{buf};
    while (true) {
        // Read the input request, in packets, and parse each packet
        std::size_t n = co_await io::async_read(ctx, conn, out_buf);
        auto data = std::string_view{buf, n};
        auto r = parser.parse_next_packet(data);
        co_return {std::move(r.value())};
    }
}
#pragma once

#include "http_server/http_response.hpp"
#include "http_server/to_buffers.hpp"
#include "io/io_context.hpp"
#include "io/connection.hpp"
#include "io/async_write.hpp"

#include <task.hpp>

auto write_http_response(io::io_context& ctx, const io::connection& conn, http_server::http_response&& resp)
        -> task<std::size_t> {
    std::vector<std::string_view> out_buffers;
    http_server::to_buffers(resp, out_buffers);
    std::size_t bytes_written{0};
    for (auto buf : out_buffers) {
        bytes_written += co_await io::async_write(ctx, conn, buf);
    }
    co_return bytes_written;
}
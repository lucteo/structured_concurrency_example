#pragma once

#include "http_server/http_response.hpp"
#include "http_server/to_buffers.hpp"
#include "io/io_context.hpp"
#include "io/connection.hpp"
#include "io/async_write.hpp"

#include <task.hpp>

auto write_http_response(io::io_context& ctx, const io::connection& conn,
        http_server::http_response resp) -> task<std::size_t> {
    { PROFILING_SCOPE_N("write_http_response -- start"); }
    std::vector<std::string_view> out_buffers;
    http_server::to_buffers(resp, out_buffers);
    std::size_t bytes_written{0};
    for (auto buf : out_buffers) {
        while (!buf.empty()) {
            auto n = co_await io::async_write(ctx, conn, buf);
            PROFILING_SCOPE_N("write_http_response -- written some data");
            bytes_written += n;
            buf = buf.substr(n);
            PROFILING_SET_TEXT_FMT(32, "cur=%d, sum=%d", int(n), int(bytes_written));
        }
    }
    co_return bytes_written;
}

#include "http_server/request_parser.hpp"
#include "http_server/create_response.hpp"
#include "http_server/to_buffers.hpp"

#include "io/async_accept.hpp"
#include "io/async_read.hpp"
#include "io/async_write.hpp"
#include "profiling.hpp"

#include <execution.hpp>
#include <task.hpp>
#include <schedulers/static_thread_pool.hpp>

#include <cstdio>
#include <algorithm>

#include <thread>
#include <chrono>

#include <signal.h>

namespace ex = std::execution;
using namespace std::chrono_literals;

void print_request(const http_server::http_request& req) {
    // Print first line
    switch (req.method_) {
    case http_server::http_method::get:
        std::printf("GET %s HTTP/1.1\n", req.uri_.c_str());
        break;
    case http_server::http_method::head:
        std::printf("HEAD %s HTTP/1.1\n", req.uri_.c_str());
        break;
    case http_server::http_method::post:
        std::printf("POST %s HTTP/1.1\n", req.uri_.c_str());
        break;
    case http_server::http_method::put:
        std::printf("PUT %s HTTP/1.1\n", req.uri_.c_str());
        break;
    case http_server::http_method::delete_method:
        std::printf("DELETE %s HTTP/1.1\n", req.uri_.c_str());
        break;
    case http_server::http_method::connect:
        std::printf("CONNECT %s HTTP/1.1\n", req.uri_.c_str());
        break;
    case http_server::http_method::options:
        std::printf("OPTIONS %s HTTP/1.1\n", req.uri_.c_str());
        break;
    case http_server::http_method::trace:
        std::printf("TRACE %s HTTP/1.1\n", req.uri_.c_str());
        break;
    case http_server::http_method::patch:
        std::printf("PATCH %s HTTP/1.1\n", req.uri_.c_str());
        break;
    }

    // Print headers
    for (const http_server::header& h : req.headers_) {
        std::printf("%s: %s\n", h.name_.c_str(), h.value_.c_str());
    }
    std::printf("\n");

    // Print the body
    std::printf("%s\n", req.body_.c_str());
}

//! Handles one connection from the client
task<bool> handle_connection(io::io_context& ctx, io::connection conn) {
    std::vector<std::string_view> out_buffers;
    try {
        // Read the input request
        http_server::request_parser parser;
        std::optional<http_server::http_request> req;
        char buf[1024];
        io::out_buffer out_buf{buf};
        while (!req) {
            // Read the input request, in packets, and parse each packet
            std::size_t n = co_await io::async_read(ctx, conn, out_buf);
            auto data = std::string_view{buf, n};
            auto r = parser.parse_next_packet(data);
            req.reset();
            req.emplace(std::move(r.value()));
        }

        // Process the request
        {
            PROFILING_SCOPE_N("process request");
            std::printf("Incoming request:\n");
            print_request(req.value());
        }

        // Generate the output
        auto resp = http_server::create_response(http_server::status_code::s_200_ok);
        http_server::to_buffers(resp, out_buffers);
        for (auto buf : out_buffers) {
            /*std::size_t written =*/co_await io::async_write(ctx, conn, buf);
        }

        // Close the connection after writing the response
        conn.close();
    } catch (std::exception& e) {
        std::printf("Exception caught: %s\n", e.what());
    }
    conn.close();
    co_return true;
}

task<bool> listener(unsigned short port, io::io_context& ctx, example::static_thread_pool& pool) {
    // Create a listening socket
    io::listening_socket listen_sock;
    listen_sock.bind(port);
    listen_sock.listen();

    while (true) {
        io::connection c{-1};
        try {
            c = co_await io::async_accept(ctx, listen_sock);
        } catch (const std::exception& e) {
            std::printf("Exception caught: %s; aborting\n", e.what());
            break;
        }
        try {
            // Handle the logic for this connection
            ex::sender auto snd =
                    ex::on(pool.get_scheduler(), handle_connection(ctx, std::move(c)));
            ex::start_detached(std::move(snd));
        } catch (const std::exception& e) {
            PROFILING_SCOPE_N("error");
            std::printf("Exception caught: %s; waiting for next connection\n", e.what());
        }
    }
    co_return true;
}

static io::io_context* g_ctx{nullptr};

auto sig_handler(int signo, siginfo_t* info, void* context) -> void {
    PROFILING_SCOPE();
    PROFILING_SET_TEXT_FMT(12, "sig=%d", signo);
    if (signo == SIGTERM && g_ctx)
        g_ctx->stop();
}

auto set_sig_handler(io::io_context& ctx, int signo) -> void {
    g_ctx = &ctx;
    struct sigaction act {};
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = &sig_handler;
    int rc = sigaction(SIGTERM, &act, nullptr);
    if (rc < 0)
        throw std::system_error(std::error_code(errno, std::system_category()));
}

int main() {
    PROFILING_SCOPE();
    unsigned short port = 8080;
    // Create a pool of threads to handle most of the work
    example::static_thread_pool pool{8};

    // Create the I/O context object, used to handle async I/O
    io::io_context ctx;
    set_sig_handler(ctx, SIGTERM);

    // Start a listener on our I/O execution context
    ex::sender auto snd = ex::on(ctx.get_scheduler(), listener(port, ctx, pool));
    ex::start_detached(std::move(snd));

    // Run the I/O execution context until we are stopped (by a signal)
    ctx.run();
    return 0;
}
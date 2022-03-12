
#include "read_http_request.hpp"
#include "write_http_response.hpp"
#include "handle_request.hpp"

#include "io/async_accept.hpp"
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

auto just_500_response() {
    auto resp = http_server::create_response(http_server::status_code::s_500_internal_server_error);
    return ex::just(std::move(resp));
}

//! Handles one connection from the client
auto handle_connection(io::io_context& ctx, const io::connection& conn) {
    // First read the HTTP request from the connection
    return read_http_request(ctx, conn)
           // Handle the request
           | ex::let_value([&ctx, &conn](http_server::http_request req) {
                 return handle_request(ctx, conn, std::move(req));
             })
           // If we have any errors, convert them to 500 error responses
           | ex::let_error([](std::exception_ptr) { return just_500_response(); })
           // If we are somehow cancelled, issue a 500 error response
           | ex::let_stopped([]() { return just_500_response(); })
           // Write the response back to the client
           | ex::let_value([&ctx, &conn](http_server::http_response resp) {
                 return write_http_response(ctx, conn, std::move(resp));
             });
}

auto listener(unsigned short port, io::io_context& ctx, example::static_thread_pool& pool)
        -> task<bool> {
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
            ex::sender auto snd =                                //
                    ex::transfer_just(pool.get_scheduler())      //
                    | ex::let_value([&ctx, c = std::move(c)]() { //
                          return handle_connection(ctx, c);
                      });
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
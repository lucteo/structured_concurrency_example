
#include "conn_data.hpp"
#include "read_http_request.hpp"
#include "write_http_response.hpp"
#include "handle_request.hpp"
#include "profiling.hpp"
#include "io/async_accept.hpp"

#include <execution.hpp>
#include <task.hpp>
#include <schedulers/static_thread_pool.hpp>

#include <signal.h>

namespace ex = std::execution;

//! Returns a sender for an HTTP response with 500 status code
auto just_500_response() {
    auto resp = http_server::create_response(http_server::status_code::s_500_internal_server_error);
    return ex::just(std::move(resp));
}

//! Handles one connection from the client
auto handle_connection(const conn_data& cdata) {
    // First read the HTTP request from the connection
    return read_http_request(cdata.io_ctx_, cdata.conn_)
           // Move to the worker pool
           | ex::transfer(cdata.pool_.get_scheduler())
           // Handle the request
           | ex::let_value([&cdata](http_server::http_request req) {
                 return handle_request(cdata, std::move(req));
             })
           // If we have any errors, convert them to 500 error responses
           | ex::let_error([](std::exception_ptr) { return just_500_response(); })
           // If we are somehow cancelled, issue a 500 error response
           | ex::let_stopped([]() { return just_500_response(); })
           // Write the response back to the client
           | ex::let_value([&cdata](http_server::http_response resp) {
                 return write_http_response(cdata.io_ctx_, cdata.conn_, std::move(resp));
             });
}

auto listener(unsigned short port, io::io_context& ctx, example::static_thread_pool& pool)
        -> task<bool> {
    // Create a listening socket
    io::listening_socket listen_sock;
    listen_sock.bind(port);
    listen_sock.listen();

    while (!ctx.is_stopped()) {
        // Accept one incoming connection
        io::connection conn = co_await io::async_accept(ctx, listen_sock);

        PROFILING_SCOPE_N("connection accepted");

        // Create a connection data object with important objects for the connection
        conn_data data{std::move(conn), ctx, pool};

        // Handle the logic for this connection
        ex::sender auto snd =                                //
                ex::just()                                   //
                | ex::let_value([data = std::move(data)]() { //
                      return handle_connection(data);
                  });
        ex::start_detached(std::move(snd));
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

auto get_main_sender() {
    return ex::just() | ex::then([] {
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
    });
}

auto main() -> int {
    std::this_thread::sync_wait(get_main_sender());
    return 0;
}
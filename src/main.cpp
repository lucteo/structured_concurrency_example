
#include "http_server/request_parser.hpp"
#include "http_server/create_response.hpp"
#include "http_server/to_buffers.hpp"

#include <execution.hpp>

#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/write.hpp>
#include <asio/thread_pool.hpp>
#include <cstdio>
#include <iostream>
#include <algorithm>

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

asio::const_buffer sv_to_asio(std::string_view in) {
    return asio::buffer(in);
}

void to_asio_buffers(const std::vector<std::string_view> in, std::vector<asio::const_buffer>& out) {
    out.reserve(in.size());
    std::transform(in.begin(), in.end(), std::back_inserter(out), &sv_to_asio);
}

//! Handles one connection from the client
asio::awaitable<void> handle_connection(asio::ip::tcp::socket socket) {
    std::vector<std::string_view> out_buffers;
    std::vector<asio::const_buffer> out_asio_buffers;
    try {
        // Read the input request
        http_server::request_parser parser;
        std::optional<http_server::http_request> req;
        char data_buf[1024];
        while (!req) {
            // Read the input request, in packets, and parse each packet
            std::size_t n =
                    co_await socket.async_read_some(asio::buffer(data_buf), asio::use_awaitable);
            std::string_view data{data_buf, n};
            auto r = parser.parse_next_packet(data);
            req.reset();
            req.emplace(std::move(r.value()));
        }

        // Process the request
        std::printf("Incoming request:\n");
        print_request(req.value());

        // Generate the output
        auto resp = http_server::create_response(http_server::status_code::s_200_ok);
        http_server::to_buffers(resp, out_buffers);
        to_asio_buffers(out_buffers, out_asio_buffers);
        co_await asio::async_write(socket, out_asio_buffers, asio::use_awaitable);
        std::printf("Done handling request.\n");

        // Close the connection after writing the response
        socket.close();
    } catch (std::exception& e) {
        std::printf("Exception caught: %s\n", e.what());
    }
}

//! Start the listener. This will accept the incoming connections.
asio::awaitable<void> listener(unsigned short port, asio::thread_pool& pool) {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor{executor, {asio::ip::tcp::v4(), port}};
    for (;;) {
        asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        // Handle the logic for this connection
        asio::co_spawn(pool, handle_connection(std::move(socket)), asio::detached);
    }
}

int main() {
    unsigned short port = 8080;
    try {
        // Create a pool of threads to handle most of the wor
        asio::thread_pool pool{8};
        // Create an I/O context, with just one thread performing the I/O
        asio::io_context io_context{1};
        // When receiving SIGINT and SIGTERM, close the I/O context
        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) { io_context.stop(); });

        // Start listening on our port
        asio::co_spawn(io_context, listener(port, pool), asio::detached);

        // Run the I/O job on the main thread
        io_context.run();
    } catch (const std::exception& e) {
        std::printf("Exception caught: %s\n", e.what());
    }
    return 0;
}
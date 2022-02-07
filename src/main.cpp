
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

//! Handles one connection from the client
asio::awaitable<void> handle_connection(asio::ip::tcp::socket socket) {
    try {
        char data[1024];
        while (true) {
            // Read the input request
            std::size_t n =
                    co_await socket.async_read_some(asio::buffer(data), asio::use_awaitable);
            (void)n;

            std::printf("Input request:\n%s\n", data);

            // Generate the output
            co_await asio::async_write(socket,
                    asio::buffer("HTTP/1.1 200 OK\nConnection: close\n\n"), asio::use_awaitable);
            std::printf("Done handling request.\n");
        }
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
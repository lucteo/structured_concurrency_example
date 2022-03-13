#pragma once

#include "http_server/http_request.hpp"
#include "http_server/http_response.hpp"
#include "http_server/create_response.hpp"
#include "io/io_context.hpp"
#include "io/connection.hpp"
#include "conn_data.hpp"
#include "parsed_uri.hpp"
#include "profiling.hpp"

#include <task.hpp>

#include <cstdio>
#include <thread>
#include <chrono>

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
    if (!req.body_.empty())
        std::printf("Body size: %d\n", int(req.body_.size()));
}

auto handle_request(const conn_data& cdata, http_server::http_request req)
        -> task<http_server::http_response> {
    PROFILING_SCOPE();
    std::printf("Incoming request:\n");
    print_request(req);
#if HAS_OPENCV
    std::printf("we have OpenCV\n");
#endif
    auto puri = parse_uri(req.uri_);
    std::printf("Path: %s\n", std::string(puri.path_).c_str());
    for (auto p : puri.params_)
        std::printf("  %s=%s\n", std::string(p.name_).c_str(), std::string(p.value_).c_str());
    std::this_thread::sleep_for(1s);
    co_return http_server::create_response(http_server::status_code::s_200_ok);
}
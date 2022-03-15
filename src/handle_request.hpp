#pragma once

#include "handle_transform_requests.hpp"
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

namespace ex = std::execution;

auto handle_request(const conn_data& cdata, http_server::http_request req)
        -> task<http_server::http_response> {
    PROFILING_SCOPE();

#if HAS_OPENCV
    auto puri = parse_uri(req.uri_);
    std::printf("URI path: '%s'\n", std::string(puri.path_).c_str());
    if (puri.path_ == "/transform/blur")
        co_return handle_blur(cdata, std::move(req), puri);
    else if (puri.path_ == "/transform/adaptthresh")
        co_return handle_adaptthresh(cdata, std::move(req), puri);
    else if (puri.path_ == "/transform/reducecolors")
        co_return handle_reducecolors(cdata, std::move(req), puri);
    else if (puri.path_ == "/transform/cartoonify")
        co_return handle_cartoonify(cdata, std::move(req), puri);
    else if (puri.path_ == "/transform/oilpainting")
        co_return handle_oilpainting(cdata, std::move(req), puri);
    else if (puri.path_ == "/transform/contourpaint")
        co_return handle_contourpaint(cdata, std::move(req), puri);
#endif
    co_return http_server::create_response(http_server::status_code::s_404_not_found);
}
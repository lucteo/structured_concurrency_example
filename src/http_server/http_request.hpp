#pragma once

#include "headers.hpp"

#include <string>
#include <vector>

namespace http_server {

//! The method that an HTTP request can have. Most requests will probably have `get` or `post`
//! methods.
enum class http_method {
    get,
    head,
    post,
    put,
    delete_method,
    connect,
    options,
    trace,
    patch,
};

//! Structure describing an HTTP request coming from the clients.
//! We aim for a simple representation here, not the most efficient one.
struct http_request {
    //! The method of the request; `GET`, `POST`, etc.
    const http_method method_;
    //! The URI for the resource we need to access
    const std::string uri_;
    //! The headers present in the request
    const headers headers_;
    //! The body of the request, if we have one
    const std::string body_;
};
} // namespace http_server
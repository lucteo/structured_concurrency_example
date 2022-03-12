#pragma once

#include "detail/native_file_desc_t.hpp"

namespace io {

//! A connection (socket). One can perform async reads and async writes on it, then close the
//! socket.
class connection {
    detail::native_file_desc_t fd_;

public:
    connection(detail::native_file_desc_t fd)
        : fd_(fd) {}
    ~connection();
    connection(connection&& other);
    auto operator=(connection&& other) -> connection&;

    connection(const connection& other) = delete;
    auto operator=(const connection& other) -> connection& = delete;

    auto close() -> void;

    auto fd() const noexcept -> detail::native_file_desc_t { return fd_; }
};

} // namespace io
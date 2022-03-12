#pragma once

#include "detail/native_file_desc_t.hpp"

namespace io {

class listening_socket {
public:
    listening_socket();
    ~listening_socket();
    listening_socket(listening_socket&& other);
    auto operator=(listening_socket&& other) -> listening_socket&;

    listening_socket(const listening_socket& other) = delete;
    auto operator=(const listening_socket& other) -> listening_socket& = delete;

    auto bind(int port) -> void;
    auto listen() -> void;

    auto fd() const -> detail::native_file_desc_t { return fd_; }

private:
    detail::native_file_desc_t fd_;
};

} // namespace io
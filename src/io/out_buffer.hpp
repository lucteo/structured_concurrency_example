#pragma once

#include <cstdint>

namespace io {

//! Somehow similar to std::sting_view, but mutable.
//! Used as output of the read operations
class out_buffer {
    char* data_;
    std::size_t size_;

public:
    out_buffer(char* data, std::size_t size) noexcept
        : data_(data)
        , size_(size) {}

    template <typename T, std::size_t N>
    out_buffer(T (&data)[N]) noexcept
        : data_(data)
        , size_(N * sizeof(T)) {}

    template <typename T, std::size_t N>
    out_buffer(T (&data)[N], std::size_t max_size) noexcept
        : data_(data)
        , size_(std::min(N * sizeof(T), max_size)) {}

    out_buffer(std::string& str) noexcept
        : data_(str.data())
        , size_(str.capacity()) {}

    auto data() const noexcept -> char* { return data_; }
    auto size() const noexcept -> std::size_t { return size_; }
};

} // namespace io

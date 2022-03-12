#pragma once

namespace io::detail {
//! The type of operation we are registering
enum class oper_type {
    read,
    write,
};
} // namespace io::detail
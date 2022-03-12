#include "connection.hpp"

#include <unistd.h>

namespace io {

connection::~connection() {
    if (fd_ > 0)
        ::close(fd_);
}
connection::connection(connection&& other)
    : fd_(other.fd_) {
    other.fd_ = 0;
}
auto connection::operator=(connection&& other) -> connection& {
    fd_ = other.fd_;
    other.fd_ = 0;
    return *this;
}

auto connection::close() -> void {
    ::close(fd_);
    fd_ = 0;
}

} // namespace io
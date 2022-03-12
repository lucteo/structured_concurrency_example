#pragma once

#include "io/io_context.hpp"
#include "io/connection.hpp"
#include <profiling.hpp>

#include <string_view>
#include <sys/socket.h>

namespace io {

namespace detail {

struct async_write_sender {
    io_context* ctx_;
    native_file_desc_t fd_;
    std::string_view data_;

    using completion_signatures = std::execution::completion_signatures< //
            std::execution::set_value_t(std::size_t),                    //
            std::execution::set_error_t(std::system_error),              //
            std::execution::set_stopped_t()>;

    template <std::execution::receiver Recv>
    class oper : oper_body_base {
        Recv recv_;
        io_context* ctx_;
        native_file_desc_t fd_;
        std::string_view data_;

        auto try_run() noexcept -> bool override {
            PROFILING_SCOPE_N("async_write::try_run");
            int rc = ::send(fd_, data_.data(), data_.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
            PROFILING_SET_TEXT_FMT(32, "fd=%d => %d", fd_, rc);
            // Is the operation complete?
            if (rc >= 0) {
                PROFILING_SCOPE_N("async_write::try_run -- DONE");
                std::size_t num_sent = static_cast<std::size_t>(rc);
                std::execution::set_value(std::move(recv_), num_sent);
                return true;
            }
            // Is the operation still in progress?
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == EALREADY)
                return false;
            // General failure
            PROFILING_SCOPE_N("async_write::try_run -- FAIL");
            auto err = std::error_code(errno, std::system_category());
            std::execution::set_error(std::move(recv_), err);
            return true;
        }
        auto set_stopped() noexcept -> void override {
            std::execution::set_stopped(std::move(recv_));
        }

    public:
        oper(Recv&& recv, io_context* ctx, native_file_desc_t fd, std::string_view data)
            : recv_(std::move(recv))
            , ctx_(ctx)
            , fd_(fd)
            , data_(data) {}

        friend void tag_invoke(std::execution::start_t, oper& self) noexcept {
            PROFILING_SCOPE_N("async_write::start");
            try {
                self.ctx_->get_scheduler().add_io_oper(self.fd_, oper_type::write, &self);
            } catch (...) {
                auto err = std::make_error_code(std::errc::operation_not_permitted);
                std::execution::set_error(std::move(self.recv_), err);
            }
        }
    };

    template <std::execution::receiver Recv>
    friend auto tag_invoke(std::execution::connect_t, async_write_sender&& self, Recv&& recv)
            -> oper<std::decay_t<Recv>> {
        return {std::forward<Recv>(recv), self.ctx_, self.fd_, self.data_};
    }
};
} // namespace detail

inline auto async_write(io_context& ctx, const connection& c, std::string_view data)
        -> detail::async_write_sender {
    return {&ctx, c.fd(), data};
}

} // namespace io
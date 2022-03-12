#pragma once

#include "detail/poll_io_loop.hpp"
#include <senders/sender_from_ftor.hpp>

#include <execution.hpp>
#include <atomic>

namespace io {
//! A context for I/O operations.
//!
//! Can create schedulers that can be used with regular scheduler to execute arbitrary computations
//! on it. It also allows adding I/O operations to be executed in this context.
class io_context {
public:
    io_context() = default;
    io_context(io_context const&) = delete;
    auto operator=(io_context const&) -> io_context& = delete;

    //! Run the I/O context loop in the current thread.
    //! Exits after `stop()` is called.
    auto run() -> std::size_t { return io_loop_.run(); }
    //! Runs maximum one operation from our context. Returns false if there is no operation to run
    auto run_one() noexcept -> bool { return io_loop_.run_one(); }

    //! Tells the I/O loop to stop.
    auto stop() noexcept -> void { io_loop_.stop(); }

    class scheduler;

    //! Get a scheduler object associated with this I/O context
    auto get_scheduler() noexcept -> scheduler;

private:
    //! The I/O loop used as the underlying implementation of this I/O context.
    detail::poll_io_loop io_loop_;
};

class io_context::scheduler {
public:
    struct my_sender {
        io_context* context_;

        using completion_signatures = std::execution::completion_signatures< //
                std::execution::set_value_t(),                               //
                std::execution::set_error_t(std::exception_ptr),             //
                std::execution::set_stopped_t()>;

        template <class Receiver>
        class operation : detail::oper_body_base {
            detail::poll_io_loop* io_loop_;
            Receiver recv_;

            auto try_run() noexcept -> bool override {
                std::execution::set_value(std::move(recv_));
                return true;
            }
            auto set_stopped() noexcept -> void override {
                std::execution::set_stopped(std::move(recv_));
            }

        public:
            operation(detail::poll_io_loop* io_loop, Receiver&& recv)
                : io_loop_(io_loop)
                , recv_(std::move(recv)) {}

            friend void tag_invoke(std::execution::start_t, operation& self) noexcept {
                try {
                    self.io_loop_->add_non_io_oper(&self);
                } catch (...) {
                    std::execution::set_error(std::move(self.recv_), std::current_exception());
                }
            }
        };

        template <class Receiver>
        friend auto tag_invoke(std::execution::connect_t, my_sender&& self, Receiver&& recv)
                -> operation<std::decay_t<Receiver>> {
            return {&self.context_->io_loop_, std::forward<Receiver>(recv)};
        }
        friend scheduler tag_invoke(
                std::execution::get_completion_scheduler_t<std::execution::set_value_t>,
                my_sender self) {
            return {self.context_};
        }
    };

public:
    scheduler(io_context* parent)
        : context_(parent) {}
    scheduler(const scheduler& other) = default;
    scheduler(scheduler&& other) = default;
    auto operator=(const scheduler& other) -> scheduler& = default;
    auto operator=(scheduler&& other) -> scheduler& = default;
    auto operator==(const scheduler& other) const -> bool = default;

    auto context() const noexcept -> io_context* { return context_; }

    friend auto tag_invoke(std::execution::schedule_t, const scheduler& self) -> my_sender {
        return {self.context_};
    }

    //! Add an I/O operation to be executed into our context
    //! The body will be called multiple times, until the operation succeeds (body function returns
    //! true)
    auto add_io_oper(detail::native_file_desc_t fd, detail::oper_type t,
            detail::oper_body_base* oper) -> void {
        context_->io_loop_.add_io_oper(fd, t, oper);
    }

private:
    io_context* context_;
};

inline auto io_context::get_scheduler() noexcept -> scheduler { return scheduler{this}; }

} // namespace io
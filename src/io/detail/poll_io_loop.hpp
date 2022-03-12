#pragma once

#include "native_file_desc_t.hpp"
#include "oper_type.hpp"
#include "oper_body_base.hpp"

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>

namespace io::detail {

//! Class that implements an I/O loop, using `pool()` for I/O.
//!
//! One can add I/O and non-I/O events into the loop, and let the loop execute them.
//! For the I/O operations, the given actions are executed multiple times until the operation
//! completes with success.
class poll_io_loop {
public:
    poll_io_loop();

    //! Run the loop once to execute maximum one operation.
    //! If there are no operations to execute (or, no I/O completed) this will return false.
    auto run_one() -> bool;

    //! Run the loop to process operations
    //! Stops after `stop()` is called, and all operations in our queues are drained.
    auto run() -> std::size_t;

    //! Stops processing any more operations
    auto stop() noexcept -> void;

    //! Check if we were told to stop
    auto is_stopped() const noexcept -> bool {
        return should_stop_.load(std::memory_order_acquire);
    }

    //! Add an I/O operation to be executed into our loop
    //! The body will be called multiple times, until the operation succeeds (body function returns
    //! true)
    auto add_io_oper(native_file_desc_t fd, oper_type t, oper_body_base* oper) -> void;

    //! Add a non-I/O operation in our loop
    //! The body will only be executed once.
    auto add_non_io_oper(oper_body_base* oper) -> void;

private:
    //! An operation that can be executed though this loop
    struct io_oper {
        native_file_desc_t fd_;
        short events_;
        oper_body_base* body_;

        io_oper(native_file_desc_t fd, short events, oper_body_base* body)
            : fd_(fd)
            , events_(events)
            , body_(body) {}
    };

    std::atomic<bool> should_stop_{false};

    // IO and non-IO operations created by various threads, not yet consumed by our loop
    std::vector<io_oper> in_opers_;
    std::mutex in_bottleneck_;

    // input operations for which we have ownership
    std::vector<io_oper> owned_in_opers_;
    std::vector<io_oper>::const_iterator next_op_to_process_;

    // data for which we call poll; the two vectors are kept in sync
    std::vector<pollfd> poll_data_;
    std::vector<oper_body_base*> poll_opers_;
    native_file_desc_t poll_wake_fd_[2];
    std::size_t check_completions_start_idx_{0};

    auto check_in_ops() -> void;
    auto handle_one_owned_in_op() -> bool;
    auto check_for_one_io_completion() -> bool;
    auto do_poll() -> bool;
};
} // namespace io::detail

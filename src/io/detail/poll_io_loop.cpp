#include "poll_io_loop.hpp"
#include <profiling.hpp>

#include <thread>
#include <chrono>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

using namespace std::chrono_literals;

namespace io::detail {

poll_io_loop::poll_io_loop() {
    PROFILING_SCOPE();

    static constexpr std::size_t expected_max_in_ops = 128;
    static constexpr std::size_t expected_max_pending_ops = 512;
    in_opers_.reserve(expected_max_in_ops);
    owned_in_opers_.reserve(expected_max_in_ops);
    poll_data_.reserve(expected_max_pending_ops);
    poll_opers_.reserve(expected_max_pending_ops);

    int rc = pipe(poll_wake_fd_);
    if (rc != 0)
        throw std::system_error(std::error_code(errno, std::system_category()));
    fcntl(poll_wake_fd_[0], F_SETFL, O_NONBLOCK);
    fcntl(poll_wake_fd_[1], F_SETFL, O_NONBLOCK);
    PROFILING_SET_TEXT_FMT(32, "pipe created: %d <-> %d", poll_wake_fd_[0], poll_wake_fd_[1]);

    poll_data_.push_back(pollfd{poll_wake_fd_[0], POLLIN, 0});
    poll_opers_.push_back(nullptr);
}

auto poll_io_loop::run_one() -> bool {
    PROFILING_SCOPE();

    // Now try to execute some outstanding ops
    while (true) {
        // Check if we have new ops, to move them on the processing for this thread
        check_in_ops();
        PROFILING_PLOT_INT("I/O ops", int(poll_data_.size()));
        if (poll_data_.size() <= 1)
            return false;

        // Check if have any completions
        if (check_for_one_io_completion())
            return true;
        // when calling do_poll, all events in poll_data_ are checked
        if (!do_poll())
            return false;
    }
}

auto poll_io_loop::run() -> std::size_t {
    PROFILING_SCOPE();
    std::size_t num_completed{0};
    // Run as many operations as possible, until the stop signal occurs
    while (!should_stop_.load(std::memory_order_acquire)) {
        if (run_one())
            num_completed++;
        else {
            // Do we still have outstanding operations to serve?
            if (!poll_data_.empty()) {
                std::this_thread::sleep_for(0ms);

            } else {
                // Try to wait for new requests
                std::unique_lock lock{in_bottleneck_};
                if (in_opers_.empty()) {
                    PROFILING_SCOPE_N("waiting for I/O operations");
                    cv_.wait(lock);
                }
            }
        }
    }

    PROFILING_SCOPE_N("exiting I/O loop");
    // If we have a stop signal, and still have outstanding operations, cancel them
    for (oper_body_base* op_body : poll_opers_) {
        if (op_body) {
            op_body->set_stopped();
            num_completed++;
        }
    }

    return num_completed;
}

auto poll_io_loop::stop() noexcept -> void {
    PROFILING_SCOPE();
    should_stop_.store(true, std::memory_order_release);
}

auto poll_io_loop::add_io_oper(native_file_desc_t fd, oper_type t, oper_body_base* body) -> void {
    PROFILING_SCOPE();
    std::scoped_lock lock{in_bottleneck_};
    short event = t == oper_type::write ? POLLOUT : POLLIN;
    in_opers_.emplace_back(fd, event, body);
    cv_.notify_one();
    const char msg = 1;
    write(poll_wake_fd_[1], &msg, 1);
    PROFILING_SET_TEXT_FMT(32, "fd=%d", fd);
}

auto poll_io_loop::add_non_io_oper(oper_body_base* body) -> void {
    PROFILING_SCOPE();
    std::scoped_lock lock{in_bottleneck_};
    in_opers_.emplace_back(-1, 0, body);
    cv_.notify_one();
    const char msg = 1;
    write(poll_wake_fd_[1], &msg, 1);
}

auto poll_io_loop::check_in_ops() -> void {
    PROFILING_SCOPE();
    while (true) {
        owned_in_opers_.clear();
        {
            // Quickly steal the items from the input vector, while under the lock
            std::scoped_lock lock{in_bottleneck_};
            owned_in_opers_.swap(in_opers_);
        }

        // If we don't have any input operations, exit
        if (owned_in_opers_.empty())
            break;

        // Start the newly added operations
        {
            PROFILING_SCOPE_N("handle input opers");
            auto new_size = poll_opers_.size() + owned_in_opers_.size();
            poll_data_.reserve(new_size);
            poll_opers_.reserve(new_size);
            for (const io_oper& op : owned_in_opers_) {
                if (op.fd_ < 0) {
                    // Simply run the non-IO operations; don't care about the result
                    op.body_->try_run();
                } else {
                    // If the I/O operation did not complete instantly, add it to our lists used
                    // when polling
                    if (!op.body_->try_run()) {
                        poll_data_.push_back(pollfd{op.fd_, op.events_, 0});
                        poll_opers_.push_back(op.body_);
                    }
                }
            }
        }
        // After executing this, we might have new in operations, so loop again
    }
}

auto poll_io_loop::check_for_one_io_completion() -> bool {
    PROFILING_SCOPE();

    char msg{0};
    while (read(poll_wake_fd_[0], &msg, 1) > 0)
        ;

    for (std::size_t i = check_completions_start_idx_; i < poll_data_.size(); i++) {
        pollfd& p = poll_data_[i];
        if ((p.events & p.revents) != 0) {
            oper_body_base* body = poll_opers_[i];
            if (body && body->try_run()) {
                // If the operation is finally complete, remove it from the list
                poll_data_.erase(poll_data_.begin() + i);
                poll_opers_.erase(poll_opers_.begin() + i);
                check_completions_start_idx_ = i;
                return true;
            }
        }
    }
    check_completions_start_idx_ = poll_data_.size();
    return false;
}
auto poll_io_loop::do_poll() -> bool {
    PROFILING_SCOPE();

    // Clear the output field in poll_data_
    for (pollfd& p : poll_data_)
        p.revents = 0;

    while (true) {
        // Perform the poll on all the poll data that we have
        int rc = poll(poll_data_.data(), poll_data_.size(), -1);
        // int rc = poll(poll_data_.data(), poll_data_.size(), 10);

#if PROFILING_ENABLED
        char buf[256];
        char* pbuf = buf;
        std::size_t remaining = 256;
        auto n = std::snprintf(pbuf, remaining, "fds:");
        remaining -= n;
        pbuf += n;
        for (auto p : poll_data_) {
            if (remaining > 0) {
                n = std::snprintf(pbuf, remaining, " %d (%d)", p.fd, int(p.events));
                remaining -= n;
                pbuf += n;
            }
        }
        if (remaining > 0) {
            std::snprintf(pbuf, remaining, " => %d", rc);
        }
        PROFILING_SET_TEXT(buf);
#endif

        if (rc >= 0) {
            // Call to `poll()` succeeded
            check_completions_start_idx_ = 0; // skip notification file
            return true;
        }
        // Failure?
        if (errno != EINVAL)
            return false;
    }
}

} // namespace io::detail
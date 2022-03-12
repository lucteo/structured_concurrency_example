#pragma once

namespace io::detail {

//! Base class to represent the actions of an operation to be passed to an I/O loop
struct oper_body_base {
    ~oper_body_base() = default;
    //! Called to run the operation. If this returns bool, the operation needs to be retried (for
    //! I/O operations).
    virtual auto try_run() noexcept -> bool = 0;
    //! Called to announce that the operation was cancelled.
    virtual auto set_stopped() noexcept -> void = 0;
};

} // namespace io::detail
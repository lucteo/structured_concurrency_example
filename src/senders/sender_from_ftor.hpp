#pragma once

#include <execution.hpp>

namespace senders {
template <typename Sig, typename Fn>
struct sender_from_ftor {
    Fn f_;

    using completion_signatures = Sig;

    template <typename Recv>
    struct oper {
        Recv recv_;
        Fn f_;

        friend void tag_invoke(std::execution::start_t, oper& self) noexcept {
            ((Fn &&) self.f_)((Recv &&) self.recv_);
        }
    };

    template <typename Recv>
    friend auto tag_invoke(std::execution::connect_t, sender_from_ftor&& self, Recv&& recv)
            -> oper<std::decay_t<Recv>> {
        return {std::forward<Recv>(recv), (Fn &&) self.f_};
    }
};

template <typename Sig, typename Fn>
auto make_sender_from_ftor(Fn&& f) -> sender_from_ftor<Sig, Fn> {
    return {std::forward<Fn>(f)};
}
} // namespace senders
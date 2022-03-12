#pragma once

#ifdef PROFILING_ENABLED

#include <tracy_interface.hpp>

#include <cstdio>
#include <string_view>

#define __PROFILING_IMPL_CONCAT2(x, y) x##y
#define __PROFILING_IMPL_CONCAT(x, y) __PROFILING_IMPL_CONCAT2(x, y)

//! Helper macro to generate static location pointers
#define __PROFILING_LOC(name, fun, file, line, color)                                              \
    ([](const char* n, const char* fn, const char* f, uint32_t l,                                  \
             uint32_t col) -> tracy_interface::location* {                                         \
        static tracy_interface::location loc{n, fn, f, l, col};                                    \
        return &loc;                                                                               \
    }(name, fun, file, line, color))

//! Define a simple scope; the name of the function will be shown
#define PROFILING_SCOPE()                                                                          \
    profiling::profiling_zone __PROFILING_IMPL_CONCAT(__profiling_scope, __LINE__) {               \
        __PROFILING_LOC(nullptr, __FUNCTION__, __FILE__, __LINE__, 0)                              \
    }

//! Define a scope with the given name (static)
#define PROFILING_SCOPE_N(static_name)                                                             \
    profiling::profiling_zone __PROFILING_IMPL_CONCAT(__profiling_scope, __LINE__) {               \
        __PROFILING_LOC((static_name), __FUNCTION__, __FILE__, __LINE__, 0)                        \
    }

//! Define a scope with default name and with the given color
#define PROFILING_SCOPE_C(color)                                                                   \
    profiling::profiling_zone __PROFILING_IMPL_CONCAT(__profiling_scope, __LINE__) {               \
        __PROFILING_LOC(nullptr, __FUNCTION__, __FILE__, __LINE__, (color))                        \
    }

//! Define a scope with the given name (static) and color
#define PROFILING_SCOPE_NC(static_name, color)                                                     \
    profiling::profiling_zone __PROFILING_IMPL_CONCAT(__profiling_scope, __LINE__) {               \
        __PROFILING_LOC((static_name), __FUNCTION__, __FILE__, __LINE__, (color))                  \
    }

//! Sets a dynamic text to the current zone
#define PROFILING_SET_TEXT(text) tracy_interface::set_text(text)
//! Sets a dynamic text, using argument formatting, to the current zone
#define PROFILING_SET_TEXT_FMT(max_len, fmt, args...)                                              \
    char __PROFILING_IMPL_CONCAT(__profiling_buffer, __LINE__)[max_len];                           \
    std::snprintf(__PROFILING_IMPL_CONCAT(__profiling_buffer, __LINE__), max_len, fmt, args);      \
    tracy_interface::set_text(                                                                     \
            std::string_view{__PROFILING_IMPL_CONCAT(__profiling_buffer, __LINE__),                \
                    strlen(__PROFILING_IMPL_CONCAT(__profiling_buffer, __LINE__))})

//! Creates a Tracy plot with the given integer value
#define PROFILING_PLOT_INT(static_plot_name, int_val)                                              \
    tracy_interface::set_plot_value_int((static_plot_name), (int_val));

//! Creates a Tracy plot with the given integer value
#define PROFILING_PLOT_FLOAT(static_plot_name, float_val)                                          \
    tracy_interface::set_plot_value_float((static_plot_name), (float_val));

namespace profiling {
//! A profiling (scoped) zone. This needs to nest well with the other zones on
//! the same thread.
struct profiling_zone {
    profiling_zone(const profiling_zone&) = delete;
    profiling_zone(profiling_zone&&) = delete;
    profiling_zone& operator=(const profiling_zone&) = delete;
    profiling_zone& operator=(profiling_zone&&) = delete;

    explicit profiling_zone(const tracy_interface::location* loc) {
        tracy_interface::emit_zone_begin(loc);
    }
    ~profiling_zone() { tracy_interface::emit_zone_end(); }
};

} // namespace profiling

#else

#define PROFILING_SCOPE()                                 /*nothing*/
#define PROFILING_SCOPE_N(static_name)                    /*nothing*/
#define PROFILING_SCOPE_NC(static_name, color)            /*nothing*/
#define PROFILING_SET_TEXT(text)                          /*nothing*/
#define PROFILING_SET_TEXT_FMT(max_len, fmt, args...)     /*nothing*/
#define PROFILING_PLOT_INT(static_plot_name, int_val)     /*nothing*/
#define PROFILING_PLOT_FLOAT(static_plot_name, float_val) /*nothing*/

#endif
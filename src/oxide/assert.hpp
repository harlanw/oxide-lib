#pragma once

#include <cstring>
#include <utility>

#include <oxide/compiler/hints.hpp>
#include <oxide/compiler/location.hpp>
#include <oxide/fmt/format_args.hpp>
#include <oxide/macros.hpp>
#include <oxide/str/base.hpp>

// clang-format off
#define OXIDE_DETAIL_ASSERT(expr, ...) \
    do { \
        if (!static_cast<bool>(expr)) { \
            ::oxide::detail::AssertAt{ \
                ::oxide::SourceLocation::current(), \
                #expr, \
                __VA_ARGS__ \
            }; \
        } \
    } while (false)

#define OXIDE_ASSERT(...) OXIDE_DETAIL_ASSERT(__VA_ARGS__, )

#define OXIDE_DETAIL_ASSERT_COMPARE(op, lhs, rhs, ...) \
    do { \
        auto&& OXIDE_MANGLE_NAME(lhs_) = (lhs); \
        auto&& OXIDE_MANGLE_NAME(rhs_) = (rhs); \
        if (!static_cast<bool>(OXIDE_MANGLE_NAME(lhs_) op OXIDE_MANGLE_NAME(rhs_))) { \
            ::oxide::detail::AssertAt{ \
                ::oxide::SourceLocation::current(), \
                #lhs " " #op " " #rhs, \
                __VA_ARGS__ \
            }; \
        } \
    } while (false)

#define OXIDE_ASSERT_EQ(...) OXIDE_DETAIL_ASSERT_COMPARE(==, __VA_ARGS__, )
#define OXIDE_ASSERT_NE(...) OXIDE_DETAIL_ASSERT_COMPARE(!=, __VA_ARGS__, )

#ifdef NDEBUG
    #define OXIDE_DEBUG_ASSERT(...) \
        do { if (false) { OXIDE_ASSERT(__VA_ARGS__); } } while (false)

    #define OXIDE_DEBUG_ASSERT_EQ(...) \
        do { if (false) { OXIDE_ASSERT_EQ(__VA_ARGS__); } } while (false)

    #define OXIDE_DEBUG_ASSERT_NE(...) \
        do { if (false) { OXIDE_ASSERT_NE(__VA_ARGS__); } } while (false)
#else
    #define OXIDE_DEBUG_ASSERT(...) OXIDE_ASSERT(__VA_ARGS__)
    #define OXIDE_DEBUG_ASSERT_EQ(...) OXIDE_ASSERT_EQ(__VA_ARGS__)
    #define OXIDE_DEBUG_ASSERT_NE(...) OXIDE_ASSERT_NE(__VA_ARGS__)
#endif
// clang-format on

namespace oxide::detail {

[[noreturn]] OXIDE_COLD void
fail_fast(const SourceLocation& location, Str base, Str extra = "") noexcept;

[[noreturn]] OXIDE_COLD void
assertion_failed(const SourceLocation& location, Str expression, Str message = "") noexcept;

struct AssertAt final {
    // Construction reports the assertion failure and never returns.
    [[noreturn]] AssertAt(const SourceLocation& location, Str expression) noexcept {
        assertion_failed(location, expression);
    }

    [[noreturn]] AssertAt(const SourceLocation& location, Str expression, Str message) noexcept {
        assertion_failed(location, expression, message);
    }

    [[noreturn]] AssertAt(
        const SourceLocation& location,
        Str expression,
        const char* message
    ) noexcept {
        if (message == nullptr) {
            assertion_failed(location, expression);
        }
        assertion_failed(location, expression, Str::from_ref(*message, std::strlen(message)));
    }

    template <typename... Args>
    [[noreturn]] AssertAt(
        const SourceLocation& location,
        Str expression,
        fmt::FormatString<Args...> format_string,
        Args&&... args
    ) noexcept {
        char message[1024];
        const auto formatted = fmt::
            format_to_n(message, sizeof(message), format_string, std::forward<Args>(args)...);
        const usize len = formatted.size < sizeof(message) ? formatted.size : sizeof(message);
        assertion_failed(location, expression, Str::from_ref(message[0], len));
    }
};

} // namespace oxide::detail

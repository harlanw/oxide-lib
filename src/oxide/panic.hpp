
#pragma once

#include <oxide/compiler/hints.hpp>
#include <oxide/compiler/location.hpp>
#include <oxide/assert.hpp>
#include <oxide/fmt/format.hpp>
#include <oxide/str.hpp>

// clang-format off
#define OXIDE_PANIC(...) \
    ::oxide::detail::PanicAt{ \
        ::oxide::SourceLocation::current(), \
        ::oxide::detail::PANIC_MESSAGE \
    }(__VA_ARGS__)

#define OXIDE_UNREACHABLE(...) \
    ::oxide::detail::PanicAt{ \
        ::oxide::SourceLocation::current(), \
        ::oxide::detail::UNREACHABLE_MESSAGE \
    }(__VA_ARGS__)

#define OXIDE_TODO(...) \
    ::oxide::detail::PanicAt{ \
        ::oxide::SourceLocation::current(), \
        ::oxide::detail::TODO_MESSAGE \
    }(__VA_ARGS__)

#define OXIDE_UNIMPLEMENTED(...) \
    ::oxide::detail::PanicAt{ \
        ::oxide::SourceLocation::current(), \
        ::oxide::detail::UNIMPLEMENTED_MESSAGE \
    }(__VA_ARGS__)
// clang-format on

namespace oxide {

namespace detail {

inline constexpr char PANIC_MESSAGE[] = "explicit panic";
inline constexpr char UNREACHABLE_MESSAGE[] = "internal error: entered unreachable code";
inline constexpr char TODO_MESSAGE[] = "not yet implemented";
inline constexpr char UNIMPLEMENTED_MESSAGE[] = "not implemented";

struct PanicAt final {
    SourceLocation location;
    Str default_message;

    [[noreturn]] void operator()() const noexcept { fail_fast(location, default_message); }

    [[noreturn]] void with_message(Str message) const noexcept {
        fail_fast(location, default_message, message);
    }

    template <typename... Args>
    [[noreturn]] void
    operator()(fmt::FormatString<Args...> format_string, Args&&... args) const noexcept {
        auto message = fmt::format(format_string, std::forward<Args>(args)...);
        with_message(message.as_str());
    }
};

} // namespace detail

[[noreturn]] OXIDE_COLD inline void panic(const SourceLocation& location, Str msg) noexcept {
    detail::fail_fast(location, msg);
}

[[noreturn]] OXIDE_COLD inline void
unreachable(Str msg = "", SourceLocation location = SourceLocation::current()) noexcept {
    detail::PanicAt{location, detail::UNREACHABLE_MESSAGE}.with_message(msg);
}

[[noreturn]] OXIDE_COLD inline void
todo(Str msg = "", SourceLocation location = SourceLocation::current()) noexcept {
    detail::PanicAt{location, detail::TODO_MESSAGE}.with_message(msg);
}

[[noreturn]] OXIDE_COLD inline void
unimplemented(Str msg = "", SourceLocation location = SourceLocation::current()) noexcept {
    detail::PanicAt{location, detail::UNIMPLEMENTED_MESSAGE}.with_message(msg);
}

} // namespace oxide

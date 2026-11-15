#pragma once

#include <iterator>
#include <memory>
#include <utility>

#include <oxide/fmt/format.hpp>
#include <oxide/io/error.hpp>

namespace oxide::io {

// Returns true if stdout is attached to a terminal.
[[nodiscard]] bool is_terminal() noexcept;

namespace detail {

// Formatting writes into bounded storage rather than an intermediate String.
class PrintBuffer final {
public:
    using value_type = char;

    void push_back(char value) {
        _data[_len++] = value;
        if (value == '\n' || _len == sizeof(_data)) {
            flush();
        }
    }

    void flush();

private:
    char _data[1024];
    usize _len{0};
};

using PrintFn = void (*)(PrintBuffer&, const void*);

} // namespace detail

// Internal print entry point. Formatting runs once while stdout is locked.
// Output errors panic; a closed stdout is discarded, matching Rust's stdio.
// Locking and output can block; this is not a hard real-time operation.
void _print(detail::PrintFn format, const void* args);

template <typename Args>
void _print(const Args& args) {
    _print(
        [](detail::PrintBuffer& buffer, const void* context) {
            (*static_cast<const Args*>(context))(std::back_inserter(buffer));
        },
        std::addressof(args)
    );
}

} // namespace oxide::io

namespace oxide {

inline void println() {
    io::_print([](auto out) {
        *out++ = '\n';
    });
}

// Uses the existing formatting backend. OXIDE_PRINTLN also checks literals at
// compile time on C++17, where the function's backend may check at runtime.
template <typename... Args>
void println(fmt::FormatString<Args...> format_string, Args&&... args) {
    io::_print([&](auto out) {
        out = fmt::format_to(std::move(out), format_string, std::forward<Args>(args)...);
        *out++ = '\n';
    });
}

template <typename Format, typename... Args>
void println(const fmt::Arguments<Format, Args...>& args) {
    io::_print([&](auto out) {
        out = fmt::format_to(std::move(out), args);
        *out++ = '\n';
    });
}

} // namespace oxide

// Prints a checked format string literal and its arguments, followed by '\n'.
// OXIDE_PRINTLN() prints only '\n'. Example: OXIDE_PRINTLN("value: {}", value).
// OXIDE_FORMAT_ARGS packages borrowed arguments without formatting them. It
// provides C++17 compile-time checking and left-to-right argument evaluation;
// println then uses _print to format while stdout is locked and append the newline.
// No intermediate formatted String is constructed.
#define OXIDE_PRINTLN(...) ::oxide::println(OXIDE_FORMAT_ARGS(__VA_ARGS__))

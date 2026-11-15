#pragma once

#include <oxide/compiler/macros.hpp>

#if __cplusplus >= 202002L && OXIDE_HAS_INCLUDE(<format>) // clang-format off
    #include <format>

    #if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
        #define OXIDE_USE_STDFORMAT 1
    #endif
#endif

#ifndef OXIDE_USE_STDFORMAT
    #define OXIDE_USE_STDFORMAT 0
    #include <fmt/format.h>
#endif // clang-format on

#include <tuple>
#include <utility>

#include <oxide/num/types.hpp>
#include <oxide/traits.hpp>

namespace oxide::fmt::detail {

template <typename T>
struct FormatArg final {
    T& value;
};

template <typename T>
using FormatValue = Decay<T>;

template <typename T>
[[nodiscard]] constexpr FormatArg<T> make_format_arg(T& value) noexcept {
    return {value};
}

#if OXIDE_USE_STDFORMAT // clang-format off
    template <typename T>
    using BackendFormatter = std::formatter<T, char>;

    template <typename... Args>
    using BackendFormatString = std::format_string<Args...>;

    using BackendFormatError = std::format_error;
#else
    template <typename T>
    using BackendFormatter = ::fmt::formatter<T, char>;

    template <typename... Args>
    using BackendFormatString = ::fmt::format_string<Args...>;

    using BackendFormatError = ::fmt::format_error;
#endif // clang-format on

} // namespace oxide::fmt::detail

namespace oxide::fmt {

template <typename T>
struct Formatter : detail::BackendFormatter<T> {};

} // namespace oxide::fmt

#if OXIDE_USE_STDFORMAT

namespace std {

template <typename T>
struct formatter<oxide::fmt::detail::FormatArg<T>, char> :
    oxide::fmt::Formatter<oxide::fmt::detail::FormatValue<T>> {
    using Base = oxide::fmt::Formatter<oxide::fmt::detail::FormatValue<T>>;

    template <typename FormatContext>
    auto format(const oxide::fmt::detail::FormatArg<T>& arg, FormatContext& context) const {
        return Base::format(arg.value, context);
    }
};

} // namespace std

#else

namespace fmt {

template <typename T>
struct formatter<oxide::fmt::detail::FormatArg<T>, char> :
    oxide::fmt::Formatter<oxide::fmt::detail::FormatValue<T>> {
    using Base = oxide::fmt::Formatter<oxide::fmt::detail::FormatValue<T>>;

    template <typename FormatContext>
    auto format(const oxide::fmt::detail::FormatArg<T>& arg, FormatContext& context) const {
        return Base::format(arg.value, context);
    }
};

} // namespace fmt

#endif

namespace oxide::fmt {

using FormatError = detail::BackendFormatError;

template <typename OutputIt>
struct FormatToNResult final {
    OutputIt out;
    usize size;
};

template <typename... Args>
using FormatString = detail::BackendFormatString<detail::FormatArg<RemoveRef<Args>>...>;

template <typename OutputIt, typename... Args>
inline OutputIt format_to(OutputIt out, FormatString<Args...> format_string, Args&&... args) {
    // clang-format off
    #if OXIDE_USE_STDFORMAT
        return std::format_to(std::move(out), format_string, detail::make_format_arg(args)...);
    #else
        return ::fmt::format_to(std::move(out), format_string, detail::make_format_arg(args)...);
    #endif // clang-format on
}

// A checked format string and borrowed arguments, created by OXIDE_FORMAT_ARGS.
// Formatting is deferred until format_to is called. Borrowing does not extend
// argument lifetimes; see OXIDE_FORMAT_ARGS for usage with temporaries.
template <typename Format, typename... Args>
class Arguments final {
public:
    explicit Arguments(Format, Args&&... args) noexcept :
        _format(FormatString<Args...>{Format{}}),
        _args(args...) {}

    template <typename OutputIt>
    OutputIt format_to(OutputIt out) const {
        return std::apply(
            [&](auto&... args) {
                return oxide::fmt::format_to(std::move(out), _format, args...);
            },
            _args
        );
    }

private:
    FormatString<Args...> _format;
    std::tuple<Args&...> _args;
};

template <typename Format, typename... Args>
Arguments(Format, Args&&...) -> Arguments<Format, Args...>;

template <typename OutputIt, typename Format, typename... Args>
inline OutputIt format_to(OutputIt out, const Arguments<Format, Args...>& args) {
    return args.format_to(std::move(out));
}

template <typename OutputIt, typename... Args>
[[nodiscard]] inline FormatToNResult<OutputIt>
format_to_n(OutputIt out, usize count, FormatString<Args...> format_string, Args&&... args) {
    // clang-format off
    #if OXIDE_USE_STDFORMAT
        auto result = std::format_to_n(
            std::move(out),
            count,
            format_string,
            detail::make_format_arg(args)...
        );
    #else
        auto result = ::fmt::format_to_n(
            std::move(out),
            count,
            format_string,
            detail::make_format_arg(args)...
        );
    #endif // clang-format on

    return {
        std::move(result.out),
        static_cast<usize>(result.size),
    };
}

template <typename... Args>
[[nodiscard]] inline usize formatted_size(FormatString<Args...> format_string, Args&&... args) {
    // clang-format off
    #if OXIDE_USE_STDFORMAT
        return static_cast<usize>(
            std::formatted_size(format_string, detail::make_format_arg(args)...)
        );
    #else
        return static_cast<usize>(
            ::fmt::formatted_size(format_string, detail::make_format_arg(args)...)
        );
    #endif // clang-format on
}

} // namespace oxide::fmt

// Preserve a literal in a type for macros that require C++17 format checking.
#if OXIDE_USE_STDFORMAT
#define OXIDE_DETAIL_FORMAT_STRING(value) \
    [] { \
        struct Literal final { \
            constexpr operator std::string_view() const noexcept { \
                return {value, sizeof(value) - 1}; \
            } \
        }; \
        return Literal{}; \
    }()
#else
#define OXIDE_DETAIL_FORMAT_STRING(value) FMT_STRING(value)
#endif

// The final empty argument keeps the helper valid for OXIDE_FORMAT_ARGS() in
// C++17. Braced initialization evaluates each argument once, left to right.
#define OXIDE_DETAIL_FORMAT_ARGS(format_string, ...) \
    ::oxide::fmt::Arguments { \
        OXIDE_DETAIL_FORMAT_STRING(format_string), __VA_ARGS__ \
    }

// Packages a format string literal and references to its arguments for use with
// fmt::format, fmt::format_to, or oxide::println. Checks syntax and argument
// types at compile time, including C++17. Argument expressions run once, left
// to right. OXIDE_FORMAT_ARGS() represents an empty format string.
//
// The bundle itself does not format, allocate, or copy argument values.
//
// Argument lifetimes are not extended:
//
// Valid: formatting finishes before the temporary string is destroyed.
//   auto text = oxide::fmt::format(OXIDE_FORMAT_ARGS("{}", std::string{"value"}));
//
// Incorrect: storing the bundle does not keep the temporary string alive.
//   auto args = OXIDE_FORMAT_ARGS("{}", std::string{"value"}); // String destroyed here.
//   auto bad = oxide::fmt::format(args); // Uses a dangling reference: undefined behavior.
#define OXIDE_FORMAT_ARGS(...) OXIDE_DETAIL_FORMAT_ARGS("" __VA_ARGS__, )

#undef OXIDE_USE_STDFORMAT

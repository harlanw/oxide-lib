#pragma once

#include <iterator>
#include <string_view>
#include <utility>

#include <oxide/fmt/format_args.hpp>
#include <oxide/string.hpp>

#define OXIDE_FORMATTER(T, ...) \
    template <> \
    struct ::oxide::Formatter<T> { \
        template <typename ParseContext> \
        constexpr auto parse(ParseContext& context) { \
            return context.begin(); \
        } \
        template <typename FormatContext> \
        auto format(const T& value, FormatContext& context) const { \
            return ::oxide::format_to(context.out(), __VA_ARGS__); \
        } \
    }

#define OXIDE_FORMATTER_T(Type, ...) \
    template <typename T> \
    struct ::oxide::Formatter<Type<T>> { \
        template <typename ParseContext> \
        constexpr auto parse(ParseContext& context) { \
            return context.begin(); \
        } \
        template <typename FormatContext> \
        auto format(const Type<T>& value, FormatContext& context) const { \
            return ::oxide::format_to(context.out(), __VA_ARGS__); \
        } \
    }

namespace oxide::fmt {

namespace detail {

struct StringWriter final {
    using value_type = char;

    String& output;

    void push_back(char value) { output.push(value); }
};

} // namespace detail

template <typename Char>
struct Formatter<BasicStr<Char>> : Formatter<std::string_view> {
    template <typename FormatContext>
    auto format(BasicStr<Char> value, FormatContext& context) const {
        return Formatter<
            std::string_view>::format(std::string_view(value.data(), value.len()), context);
    }
};

template <>
struct Formatter<String> : Formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const String& value, FormatContext& context) const {
        return Formatter<
            std::string_view>::format(std::string_view(value.data(), value.len()), context);
    }
};

template <typename... Args>
[[nodiscard]] inline String format(FormatString<Args...> format_string, Args&&... args) {
    String result;
    detail::StringWriter writer{result};
    oxide::fmt::format_to(std::back_inserter(writer), format_string, std::forward<Args>(args)...);
    return result;
}

template <typename Format, typename... Args>
[[nodiscard]] inline String format(const Arguments<Format, Args...>& args) {
    String result;
    detail::StringWriter writer{result};
    oxide::fmt::format_to(std::back_inserter(writer), args);
    return result;
}

} // namespace oxide::fmt

#pragma once

#include <cctype>
#include <string>
#include <string_view>

#include <oxide/str/base.hpp>
#include <oxide/slice.hpp>

namespace oxide {

template <class Char>
constexpr BasicStr<Char> BasicStr<Char>::from_slice(Slice<Char> s) {
    return {s.begin(), s.len()};
}

template <class Char>
constexpr Option<BasicStr<Char>> BasicStr<Char>::from_ptr(Char* ptr, usize len) {
    return ptr == nullptr ? NONE : some(BasicStr{ptr, len});
}

template <class Char>
constexpr auto BasicStr<Char>::iter() const noexcept {
    return Slice<Char>::from_ptr(_data, _len).unwrap().iter();
}

template <class Char>
constexpr auto BasicStr<Char>::to_lowercase() const noexcept {
    return iter().map([](Char ch) {
        return std::tolower(static_cast<unsigned char>(ch));
    });
}

template <class Char>
constexpr auto BasicStr<Char>::to_uppercase() const noexcept {
    return iter().map([](Char ch) {
        return std::toupper(static_cast<unsigned char>(ch));
    });
}

template <class Char>
constexpr Char& BasicStr<Char>::operator[](usize index) const noexcept {
    OXIDE_ASSERT(index < _len, "out of bounds");
    return _data[index];
}

[[nodiscard]] constexpr Str AsStr(std::string_view value) noexcept {
    if (value.data() == nullptr && value.empty()) {
        return Str::from_ptr("", 0).unwrap();
    }
    return Str::from_ptr(value.data(), value.size()).unwrap();
}

template <typename Traits, typename Allocator>
[[nodiscard]] inline Str AsStr(const std::basic_string<char, Traits, Allocator>& value) noexcept {
    return AsStr(std::string_view(value.data(), value.size()));
}

template <typename Traits, typename Allocator>
Str AsStr(std::basic_string<char, Traits, Allocator>&&) = delete;

template <typename Traits, typename Allocator>
Str AsStr(const std::basic_string<char, Traits, Allocator>&&) = delete;

[[nodiscard]] constexpr Str AsStr(Str value) noexcept {
    return value;
}

[[nodiscard]] constexpr Str AsStr(StrMut value) noexcept {
    return Str::from_ptr(value.data(), value.len()).unwrap();
}

// Mutable conversions require mutable characters. Const-qualified borrowed
// view objects still refer to mutable characters, as with Slice<char>.
template <typename Traits, typename Allocator>
[[nodiscard]] inline StrMut AsStrMut(std::basic_string<char, Traits, Allocator>& value) noexcept {
    return StrMut::from_ptr(value.data(), value.size()).unwrap();
}

[[nodiscard]] constexpr StrMut AsStrMut(StrMut value) noexcept {
    return value;
}

} // namespace oxide

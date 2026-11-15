#pragma once

#include <memory>

#include <oxide/num/types.hpp>

namespace oxide {

template <class T>
class Option;

template <class T>
class Slice;

template <class Char>
class BasicStr {
    Char* _data;
    usize _len;

public:
    template <usize N>
    constexpr BasicStr(Char (&value)[N]) noexcept :
        _data(value),
        _len(value[N - 1] == '\0' ? N - 1 : N) {}

    static constexpr BasicStr from_slice(Slice<Char> s);

    static constexpr Option<BasicStr> from_ptr(Char* ptr, usize len);

    static constexpr BasicStr from_ref(Char& ref, usize len) noexcept {
        return {std::addressof(ref), len};
    }

    [[nodiscard]] constexpr auto iter() const noexcept;

    [[nodiscard]] constexpr bool strip_prefix(BasicStr prefix) noexcept {
        if (!starts_with(prefix)) {
            return false;
        }
        _data += prefix._len;
        _len -= prefix._len;
        return true;
    }

    [[nodiscard]] constexpr bool starts_with(char prefix) const noexcept {
        return _len > 0 && _data[0] == prefix;
    }

    [[nodiscard]] constexpr bool starts_with(BasicStr prefix) const noexcept {
        if (prefix._len > _len) {
            return false;
        }
        for (usize i = 0; i < prefix._len; ++i) {
            if (_data[i] != prefix._data[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr bool ends_with(char suffix) const noexcept {
        return _len > 0 && _data[_len - 1] == suffix;
    }

    [[nodiscard]] constexpr bool ends_with(BasicStr suffix) const noexcept {
        if (suffix._len > _len) {
            return false;
        }
        const usize offset = _len - suffix._len;
        for (usize i = 0; i < suffix._len; ++i) {
            if (_data[offset + i] != suffix._data[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr bool contains(char value) const noexcept {
        for (usize i = 0; i < _len; ++i) {
            if (_data[i] == value) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] constexpr bool contains_any(BasicStr values) const noexcept {
        for (usize i = 0; i < values._len; ++i) {
            if (contains(values._data[i])) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] constexpr bool contains_only(BasicStr values) const noexcept {
        for (usize i = 0; i < _len; ++i) {
            if (!values.contains(_data[i])) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr auto to_lowercase() const noexcept;

    [[nodiscard]] constexpr auto to_uppercase() const noexcept;

    [[nodiscard]] constexpr Char* data() const noexcept { return _data; }

    [[nodiscard]] constexpr usize len() const noexcept { return _len; }

    [[nodiscard]] constexpr bool is_empty() const noexcept { return _len == 0; }

    [[nodiscard]] constexpr Char& operator[](usize index) const noexcept;

    [[nodiscard]] constexpr bool operator==(BasicStr other) const noexcept {
        if (_len != other._len) {
            return false;
        }
        for (usize i = 0; i < _len; ++i) {
            if (!(_data[i] == other._data[i])) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr bool operator!=(BasicStr other) const noexcept {
        return !(*this == other);
    }

private:
    constexpr BasicStr(Char* data, usize len) noexcept : _data(data), _len(len) {}
};

using Str = BasicStr<const char>;

using StrMut = BasicStr<char>;

} // namespace oxide

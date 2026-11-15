#pragma once

#include <oxide/math/arithmetic.hpp>
#include <oxide/traits.hpp>

namespace oxide {

template <typename T, typename Tag, IfIntegral<T> = 0>
class StrongNum final {
    T _value{};

public:
    constexpr StrongNum() noexcept = default;

    constexpr explicit StrongNum(T value) noexcept : _value(value) {}

    [[nodiscard]] constexpr T inner() const noexcept { return _value; }

    [[nodiscard]] constexpr bool is_zero() const noexcept { return _value == 0; }

    [[nodiscard]] constexpr StrongNum saturating_add(StrongNum rhs) const noexcept {
        return StrongNum{oxide::saturating_add(_value, rhs._value)};
    }

    [[nodiscard]] constexpr StrongNum saturating_sub(StrongNum rhs) const noexcept {
        return StrongNum{oxide::saturating_sub(_value, rhs._value)};
    }

    [[nodiscard]] constexpr StrongNum saturating_mul(StrongNum rhs) const noexcept {
        return StrongNum{oxide::saturating_mul(_value, rhs._value)};
    }

    constexpr StrongNum& operator+=(StrongNum rhs) noexcept { return *this = *this + rhs; }

    constexpr StrongNum& operator-=(StrongNum rhs) noexcept { return *this = *this - rhs; }

    constexpr StrongNum& operator*=(StrongNum rhs) noexcept { return *this = *this * rhs; }

    constexpr StrongNum& operator++() noexcept { return *this += StrongNum{T{1}}; }

    constexpr StrongNum& operator--() noexcept { return *this -= StrongNum{T{1}}; }

    friend constexpr bool operator==(StrongNum a, StrongNum b) noexcept {
        return a._value == b._value;
    }

    friend constexpr bool operator!=(StrongNum a, StrongNum b) noexcept { return !(a == b); }

    friend constexpr bool operator<(StrongNum a, StrongNum b) noexcept {
        return a._value < b._value;
    }

    friend constexpr bool operator<=(StrongNum a, StrongNum b) noexcept { return !(b < a); }

    friend constexpr bool operator>(StrongNum a, StrongNum b) noexcept { return b < a; }

    friend constexpr bool operator>=(StrongNum a, StrongNum b) noexcept { return !(a < b); }
};

#define OXIDE_STRONG_NUM(Name, T) \
    namespace _ { \
    struct Name##Tag final {}; \
    } \
    using Name = ::oxide::StrongNum<T, _::Name##Tag>

} // namespace oxide

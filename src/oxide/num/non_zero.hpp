#pragma once

#include <limits>
#include <type_traits>

#include <oxide/assert.hpp>
#include <oxide/num/types.hpp>
#include <oxide/option.hpp>
#include <oxide/traits.hpp>

namespace oxide {

template <typename T, IfIntegral<T> = 0>
class NonZero final {
    static_assert(IsSame<T, RemoveCv<T>>{}, "T must not be cv-qualified");

    T _inner;

    explicit constexpr NonZero(T value) noexcept : _inner(value) {}

    [[nodiscard]] static constexpr bool mul_overflows(T a, T b) noexcept {
        if constexpr (IsUnsigned<T>{}) {
            return a > std::numeric_limits<T>::max() / b;
        } else {
            constexpr T min = std::numeric_limits<T>::min();
            constexpr T max = std::numeric_limits<T>::max();

            if (a > 0) {
                return b > 0 ? a > max / b : b < min / a;
            }
            return b > 0 ? a < min / b : a < max / b;
        }
    }

    [[nodiscard]] static constexpr T saturating_mul_value(T a, T b) noexcept {
        if (!mul_overflows(a, b)) {
            return static_cast<T>(a * b);
        }

        if constexpr (IsSigned<T>{}) {
            if ((a < 0) != (b < 0)) {
                return std::numeric_limits<T>::min();
            }
        }

        return std::numeric_limits<T>::max();
    }

public:
    static constexpr u32 BITS = std::numeric_limits<std::make_unsigned_t<T>>::digits;

    [[nodiscard]] static constexpr NonZero MIN() noexcept {
        return NonZero{IsSigned<T>{} ? std::numeric_limits<T>::min() : T{1}};
    }

    [[nodiscard]] static constexpr NonZero MAX() noexcept {
        return NonZero{std::numeric_limits<T>::max()};
    }

    [[nodiscard]] static constexpr Option<NonZero> from(T value) noexcept {
        if (value == 0) {
            return NONE;
        }
        return some(NonZero{value});
    }

    [[nodiscard]] static constexpr NonZero from_unchecked(T value) noexcept {
        OXIDE_DEBUG_ASSERT(value != 0);
        return NonZero{value};
    }

    [[nodiscard]] constexpr T get() const noexcept { return _inner; }

    [[nodiscard]] constexpr Option<NonZero> checked_add(T other) const noexcept {
        static_assert(IsUnsigned<T>{}, "checked_add requires unsigned T");

        constexpr T max = std::numeric_limits<T>::max();

        if (other > max - _inner) {
            return NONE;
        }
        return some(NonZero{static_cast<T>(_inner + other)});
    }

    [[nodiscard]] constexpr NonZero saturating_add(T other) const noexcept {
        static_assert(IsUnsigned<T>{}, "saturating_add requires unsigned T");

        constexpr T max = std::numeric_limits<T>::max();
        return NonZero{other > max - _inner ? max : static_cast<T>(_inner + other)};
    }

    [[nodiscard]] constexpr Option<NonZero> checked_mul(NonZero other) const noexcept {
        if (mul_overflows(_inner, other._inner)) {
            return NONE;
        }
        return some(NonZero{static_cast<T>(_inner * other._inner)});
    }

    [[nodiscard]] constexpr NonZero saturating_mul(NonZero other) const noexcept {
        return NonZero{saturating_mul_value(_inner, other._inner)};
    }

    [[nodiscard]] constexpr Option<NonZero> checked_pow(u32 exp) const noexcept {
        T result{1};
        T base = _inner;

        while (exp != 0) {
            if ((exp & 1) != 0) {
                if (mul_overflows(result, base)) {
                    return NONE;
                }
                result = static_cast<T>(result * base);
            }

            exp >>= 1;

            if (exp != 0) {
                if (mul_overflows(base, base)) {
                    return NONE;
                }
                base = static_cast<T>(base * base);
            }
        }

        return some(NonZero{result});
    }

    [[nodiscard]] constexpr NonZero saturating_pow(u32 exp) const noexcept {
        T result{1};
        T base = _inner;

        while (exp != 0) {
            if ((exp & 1) != 0) {
                result = saturating_mul_value(result, base);
            }

            exp >>= 1;

            if (exp != 0) {
                base = saturating_mul_value(base, base);
            }
        }

        return NonZero{result};
    }

    [[nodiscard]] constexpr Option<NonZero> checked_abs() const noexcept {
        static_assert(IsSigned<T>{}, "checked_abs requires signed T");

        if (_inner == std::numeric_limits<T>::min()) {
            return NONE;
        }
        return some(NonZero{_inner < 0 ? static_cast<T>(-_inner) : _inner});
    }

    [[nodiscard]] constexpr NonZero saturating_abs() const noexcept {
        static_assert(IsSigned<T>{}, "saturating_abs requires signed T");

        if (_inner == std::numeric_limits<T>::min()) {
            return MAX();
        }
        return NonZero{_inner < 0 ? static_cast<T>(-_inner) : _inner};
    }

    [[nodiscard]] constexpr Option<NonZero> checked_neg() const noexcept {
        static_assert(IsSigned<T>{}, "checked_neg requires signed T");

        if (_inner == std::numeric_limits<T>::min()) {
            return NONE;
        }
        return some(NonZero{static_cast<T>(-_inner)});
    }

    [[nodiscard]] constexpr NonZero saturating_neg() const noexcept {
        static_assert(IsSigned<T>{}, "saturating_neg requires signed T");

        return _inner == std::numeric_limits<T>::min() ? MAX() : NonZero{static_cast<T>(-_inner)};
    }

    [[nodiscard]] constexpr bool is_power_of_two() const noexcept {
        static_assert(IsUnsigned<T>{}, "is_power_of_two requires unsigned T");
        return (_inner & static_cast<T>(_inner - 1)) == 0;
    }

    friend constexpr bool operator==(NonZero a, NonZero b) noexcept { return a._inner == b._inner; }
    friend constexpr bool operator!=(NonZero a, NonZero b) noexcept { return !(a == b); }
    friend constexpr bool operator<(NonZero a, NonZero b) noexcept { return a._inner < b._inner; }
    friend constexpr bool operator<=(NonZero a, NonZero b) noexcept { return !(b < a); }
    friend constexpr bool operator>(NonZero a, NonZero b) noexcept { return b < a; }
    friend constexpr bool operator>=(NonZero a, NonZero b) noexcept { return !(a < b); }
};

} // namespace oxide

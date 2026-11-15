#pragma once

#include <limits>

#include <oxide/compiler/macros.hpp>
#include <oxide/traits.hpp>
#include <oxide/option.hpp>

namespace oxide {

template <typename T, IfIntegral<T> = 0>
[[nodiscard]] constexpr bool is_power_of_two(T value) noexcept {
    return value > 0 && (value & (value - 1)) == 0;
}

template <typename T, IfIntegral<T> = 0>
[[nodiscard]] constexpr Option<T> checked_add(T lhs, T rhs) noexcept { // clang-format off
    #if OXIDE_HAS_BUILTIN(__builtin_add_overflow)
        T result;
        if (__builtin_add_overflow(lhs, rhs, &result)) {
            return NONE;
        }
        return some(result);
    #else
        constexpr auto min = std::numeric_limits<T>::lowest();
        constexpr auto max = std::numeric_limits<T>::max();

        if constexpr (IsUnsigned<T>{}) {
            if (rhs > max - lhs) {
                return NONE;
            }
        } else {
            if ((rhs > 0 && lhs > max - rhs) || (rhs < 0 && lhs < min - rhs)) {
                return NONE;
            }
        }

        return some(as<T>(lhs + rhs));
    #endif // clang-format on
}

template <typename T, IfIntegral<T> = 0>
[[nodiscard]] constexpr Option<T> checked_mul(T lhs, T rhs) noexcept { // clang-format off
    #if OXIDE_HAS_BUILTIN(__builtin_mul_overflow)
        T result;
        if (__builtin_mul_overflow(lhs, rhs, &result)) {
            return NONE;
        }
        return some(result);
    #else
        constexpr auto min = std::numeric_limits<T>::lowest();
        constexpr auto max = std::numeric_limits<T>::max();

        if constexpr (IsUnsigned<T>{}) {
            if (rhs != 0 && lhs > max / rhs) {
                return NONE;
            }
        } else {
            if (lhs > 0) {
                if ((rhs > 0 && lhs > max / rhs) || (rhs < 0 && rhs < min / lhs)) {
                    return NONE;
                }
            } else if (lhs < 0) {
                if ((rhs > 0 && lhs < min / rhs) || (rhs < 0 && lhs < max / rhs)) {
                    return NONE;
                }
            }
        }

        return some(as<T>(lhs * rhs));
    #endif // clang-format on
}

template <typename T, IfIntegral<T> = 0>
[[nodiscard]] constexpr T saturating_add(T lhs, T rhs) noexcept {
    constexpr auto min = std::numeric_limits<T>::lowest();
    constexpr auto max = std::numeric_limits<T>::max();

    if (auto result = checked_add(lhs, rhs); result.is_some()) {
        return *result;
    }

    if constexpr (IsUnsigned<T>{}) {
        return max;
    } else {
        return rhs < 0 ? min : max;
    }
}

template <typename T, IfIntegral<T> = 0>
[[nodiscard]] constexpr T saturating_sub(T lhs, T rhs) noexcept { // clang-format off
    constexpr auto min = std::numeric_limits<T>::lowest();
    constexpr auto max = std::numeric_limits<T>::max();

    #if OXIDE_HAS_BUILTIN(__builtin_sub_overflow)
        T result;
        if (__builtin_sub_overflow(lhs, rhs, &result)) {
            if constexpr (IsUnsigned<T>{}) {
                return min;
            } else {
                return rhs > 0 ? min : max;
            }
        }
        return result;
    #else
        if constexpr (IsUnsigned<T>{}) {
            return lhs < rhs ? min : static_cast<T>(lhs - rhs);
        } else {
            if (rhs > 0 && lhs < min + rhs) {
                return min;
            }
            if (rhs < 0 && lhs > max + rhs) {
                return max;
            }
            return static_cast<T>(lhs - rhs);
        }
    #endif // clang-format on
}

template <typename T, IfIntegral<T> = 0>
[[nodiscard]] constexpr T saturating_mul(T lhs, T rhs) noexcept {
    constexpr auto min = std::numeric_limits<T>::lowest();
    constexpr auto max = std::numeric_limits<T>::max();

    if (auto result = checked_mul(lhs, rhs); result.is_some()) {
        return *result;
    }

    if constexpr (IsUnsigned<T>{}) {
        return max;
    } else {
        return (lhs < 0) != (rhs < 0) ? min : max;
    }
}

} // namespace oxide

#pragma once

#include <oxide/traits.hpp>

namespace oxide {

template <typename T, typename = EnableIf<IsFloatingPoint<T>{}>>
[[nodiscard]] constexpr bool is_nan(T value) noexcept {
    return value != value;
}

template <typename T, typename U>
[[nodiscard]] constexpr T as(U value) noexcept {
    static_assert(IsArithmetic<T>{});
    static_assert(IsArithmetic<U>{});

    if constexpr (IsIntegral<T>{} && IsFloatingPoint<U>{}) {
        if (oxide::is_nan(value)) {
            return T{0};
        }

        constexpr U MIN = static_cast<U>(std::numeric_limits<T>::lowest());
        constexpr U MAX = static_cast<U>(std::numeric_limits<T>::max());

        if (value <= MIN) {
            return std::numeric_limits<T>::lowest();
        }

        if (value >= MAX) {
            return std::numeric_limits<T>::max();
        }
    }

    return static_cast<T>(value);
}

} // namespace oxide

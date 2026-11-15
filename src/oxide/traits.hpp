#pragma once

#include <type_traits>
#include <utility>

#include <oxide/traits/properties.hpp>
#include <oxide/traits/transformations.hpp>

namespace oxide {

template <typename T, typename = void>
struct IsEquatable : std::false_type {
    static constexpr bool nothrow = false;
};

template <typename T>
struct IsEquatable<
    T,
    Void<decltype(static_cast<bool>(std::declval<const T&>() == std::declval<const T&>()))>> :
    std::true_type {
    static constexpr bool nothrow = noexcept(
        static_cast<bool>(std::declval<const T&>() == std::declval<const T&>())
    );
};

template <typename T, typename = void>
struct IsOrderable : std::false_type {
    static constexpr bool nothrow = false;
};

template <typename T>
struct IsOrderable<
    T,
    Void<decltype(static_cast<bool>(std::declval<const T&>() < std::declval<const T&>()))>> :
    IsEquatable<T> {
    static constexpr bool nothrow = IsEquatable<T>::nothrow
        && noexcept(static_cast<bool>(std::declval<const T&>() < std::declval<const T&>()));
};

} // namespace oxide

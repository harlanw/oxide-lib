#pragma once

#include <oxide/num/convert.hpp>
#include <oxide/num/types.hpp>
#include <oxide/traits.hpp>

namespace oxide {

template <typename Enum>
[[nodiscard]] constexpr UnderlyingType<Enum> to_underlying(Enum value) noexcept {
    return static_cast<UnderlyingType<Enum>>(value);
}

// To enable bitwise operators on enum structs:
//   enum class MyEnum : u32 { ... };
//
//   OXIDE_ENABLE_BITMASK_OPERATORS(MyEnum)
//
//   MyEnum a = MyEnum::A | MyEnum::B;
template <typename E>
constexpr bool enable_bitmask_operators(E) {
    return false;
}

template <typename E, typename T = E>
using EnableBitmask = EnableIf<enable_bitmask_operators(E{}), T>;

template <typename E>
[[nodiscard]] constexpr EnableBitmask<E> operator|(E lhs, E rhs) noexcept {
    using U = UnderlyingType<E>;
    return static_cast<E>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template <typename E>
[[nodiscard]] constexpr EnableBitmask<E> operator&(E lhs, E rhs) noexcept {
    using U = UnderlyingType<E>;
    return static_cast<E>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template <typename E>
[[nodiscard]] constexpr EnableBitmask<E> operator^(E lhs, E rhs) noexcept {
    using U = UnderlyingType<E>;
    return static_cast<E>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
}

template <typename E>
[[nodiscard]] constexpr EnableBitmask<E> operator~(E value) noexcept {
    using U = UnderlyingType<E>;
    return static_cast<E>(~static_cast<U>(value));
}

template <typename E>
constexpr EnableBitmask<E, E&> operator|=(E& lhs, E rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

template <typename E>
constexpr EnableBitmask<E, E&> operator&=(E& lhs, E rhs) noexcept {
    lhs = lhs & rhs;
    return lhs;
}

template <typename E>
constexpr EnableBitmask<E, E&> operator^=(E& lhs, E rhs) noexcept {
    lhs = lhs ^ rhs;
    return lhs;
}

#define OXIDE_ENABLE_BITMASK_OPERATORS(E) \
    constexpr bool enable_bitmask_operators(E) { \
        return true; \
    }

#define OXIDE_REEXPORT_ENUM(T, Name) static constexpr T Name = T::Name
#define OXIDE_REEXPORT_ERROR(Name) OXIDE_REEXPORT_ENUM(Error, Name)
#define OXIDE_REEXPORT_FLAGS(Name) OXIDE_REEXPORT_ENUM(Flags, Name)
#define OXIDE_REEXPORT_STATE(Name) OXIDE_REEXPORT_ENUM(State, Name)
#define OXIDE_REEXPORT_TYPE(Name) OXIDE_REEXPORT_ENUM(Type, Name)

template <typename T, IfIntegral<T> = 0>
[[nodiscard]] constexpr u32 trailing_zeros(T value) noexcept {
    using U = std::make_unsigned_t<T>;
    static_assert(sizeof(U) <= sizeof(u64));

    const U bits = static_cast<U>(value);

    if (bits == 0) {
        return std::numeric_limits<U>::digits;
    }

    if constexpr (sizeof(U) <= sizeof(u32)) {
        return as<u32>(__builtin_ctz(as<u32>(bits)));
    } else if constexpr (sizeof(U) <= sizeof(unsigned long)) {
        return as<u32>(__builtin_ctzl(as<u32>(bits)));
    } else {
        return as<u32>(__builtin_ctzll(as<u64>(bits)));
    }
}

template <typename T, IfUnsigned<T> = 0>
[[nodiscard]] constexpr T bit_mask(usize msb, usize lsb = 0) noexcept {
    constexpr usize WIDTH = std::numeric_limits<T>::digits;
    constexpr T MAX = std::numeric_limits<T>::max();

    if (msb >= WIDTH || lsb > msb) {
        return T{0};
    }

    const usize bits = msb - lsb + 1;
    return static_cast<T>((MAX >> (WIDTH - bits)) << lsb);
}

} // namespace oxide

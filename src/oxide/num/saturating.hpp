#pragma once

#include <climits>
#include <limits>
#include <type_traits>

#include <oxide/compiler/target.hpp>
#include <oxide/math/arithmetic.hpp>
#include <oxide/assert.hpp>
#include <oxide/traits.hpp>

namespace oxide {

template <typename T, IfIntegral<T> = 0>
class Saturating final {
    static_assert(IsSame<T, RemoveCv<T>>{}, "T must be unqualified");
    static_assert(CHAR_BIT == 8, "Saturating requires 8-bit bytes");

    using U = std::make_unsigned_t<T>;

    T _inner{0};

public:
    static constexpr u32 BITS = std::numeric_limits<std::make_unsigned_t<T>>::digits;
    static const Saturating MIN;
    static const Saturating MAX;

    constexpr Saturating() noexcept = default;
    explicit constexpr Saturating(T value) noexcept : _inner(value) {}

    [[nodiscard]] constexpr T get() const noexcept { return _inner; }

    [[nodiscard]] friend constexpr Saturating operator+(Saturating a, Saturating b) noexcept {
        return Saturating{oxide::saturating_add(a._inner, b._inner)};
    }

    [[nodiscard]] friend constexpr Saturating operator-(Saturating a, Saturating b) noexcept {
        return Saturating{oxide::saturating_sub(a._inner, b._inner)};
    }

    [[nodiscard]] friend constexpr Saturating operator*(Saturating a, Saturating b) noexcept {
        return Saturating{oxide::saturating_mul(a._inner, b._inner)};
    }

    [[nodiscard]] friend constexpr Saturating operator/(Saturating a, Saturating b) noexcept {
        OXIDE_ASSERT(b._inner != 0);

        if constexpr (IsSigned<T>{}) {
            if (a._inner == std::numeric_limits<T>::min() && b._inner == T{-1}) {
                return MAX();
            }
        }

        return Saturating{static_cast<T>(a._inner / b._inner)};
    }

    [[nodiscard]] friend constexpr Saturating operator%(Saturating a, Saturating b) noexcept {
        OXIDE_ASSERT(b._inner != 0);

        if constexpr (IsSigned<T>{}) {
            OXIDE_ASSERT(a._inner != std::numeric_limits<T>::min() || b._inner != T{-1});
        }

        return Saturating{static_cast<T>(a._inner % b._inner)};
    }

    constexpr Saturating& operator+=(Saturating other) noexcept { return *this = *this + other; }
    constexpr Saturating& operator-=(Saturating other) noexcept { return *this = *this - other; }
    constexpr Saturating& operator*=(Saturating other) noexcept { return *this = *this * other; }
    constexpr Saturating& operator/=(Saturating other) noexcept { return *this = *this / other; }
    constexpr Saturating& operator%=(Saturating other) noexcept { return *this = *this % other; }

    constexpr Saturating& operator+=(T other) noexcept { return *this += Saturating{other}; }
    constexpr Saturating& operator-=(T other) noexcept { return *this -= Saturating{other}; }
    constexpr Saturating& operator*=(T other) noexcept { return *this *= Saturating{other}; }
    constexpr Saturating& operator/=(T other) noexcept { return *this /= Saturating{other}; }
    constexpr Saturating& operator%=(T other) noexcept { return *this %= Saturating{other}; }

    [[nodiscard]] friend constexpr Saturating operator&(Saturating a, Saturating b) noexcept {
        return Saturating{from_bits(static_cast<U>(a.bits() & b.bits()))};
    }

    [[nodiscard]] friend constexpr Saturating operator|(Saturating a, Saturating b) noexcept {
        return Saturating{from_bits(static_cast<U>(a.bits() | b.bits()))};
    }

    [[nodiscard]] friend constexpr Saturating operator^(Saturating a, Saturating b) noexcept {
        return Saturating{from_bits(static_cast<U>(a.bits() ^ b.bits()))};
    }

    [[nodiscard]] constexpr Saturating operator~() const noexcept {
        return Saturating{from_bits(static_cast<U>(~bits()))};
    }

    constexpr Saturating& operator&=(Saturating other) noexcept { return *this = *this & other; }
    constexpr Saturating& operator|=(Saturating other) noexcept { return *this = *this | other; }
    constexpr Saturating& operator^=(Saturating other) noexcept { return *this = *this ^ other; }

    constexpr Saturating& operator&=(T other) noexcept { return *this &= Saturating{other}; }
    constexpr Saturating& operator|=(T other) noexcept { return *this |= Saturating{other}; }
    constexpr Saturating& operator^=(T other) noexcept { return *this ^= Saturating{other}; }

    [[nodiscard]] constexpr Saturating operator-() const noexcept {
        static_assert(IsSigned<T>{}, "unary negation requires signed T");

        return _inner == std::numeric_limits<T>::min()
            ? MAX()
            : Saturating{static_cast<T>(-_inner)};
    }

    [[nodiscard]] constexpr Saturating pow(u32 exp) const noexcept {
        Saturating result{T{1}};
        Saturating base{*this};

        while (exp != 0) {
            if ((exp & 1) != 0) {
                result *= base;
            }

            exp >>= 1;

            if (exp != 0) {
                base *= base;
            }
        }

        return result;
    }

    [[nodiscard]] constexpr Saturating abs() const noexcept {
        static_assert(IsSigned<T>{}, "abs requires signed T");
        return _inner < 0 ? -(*this) : *this;
    }

    [[nodiscard]] constexpr Saturating signum() const noexcept {
        static_assert(IsSigned<T>{}, "signum requires signed T");
        return Saturating{static_cast<T>((_inner > 0) - (_inner < 0))};
    }

    [[nodiscard]] constexpr bool is_positive() const noexcept {
        static_assert(IsSigned<T>{}, "is_positive requires signed T");
        return _inner > 0;
    }

    [[nodiscard]] constexpr bool is_negative() const noexcept {
        static_assert(IsSigned<T>{}, "is_negative requires signed T");
        return _inner < 0;
    }

    [[nodiscard]] constexpr bool is_power_of_two() const noexcept {
        static_assert(IsUnsigned<T>{}, "is_power_of_two requires unsigned T");
        return _inner != 0 && (_inner & static_cast<T>(_inner - 1)) == 0;
    }

    [[nodiscard]] constexpr u32 count_ones() const noexcept {
        U value = bits();
        u32 count = 0;

        while (value != 0) {
            value = static_cast<U>(value & (value - 1));
            ++count;
        }

        return count;
    }

    [[nodiscard]] constexpr u32 count_zeros() const noexcept { return BITS - count_ones(); }

    [[nodiscard]] constexpr u32 trailing_zeros() const noexcept {
        U value = bits();
        if (value == 0) {
            return BITS;
        }

        u32 count = 0;

        while ((value & U{1}) == 0) {
            value >>= 1;
            ++count;
        }

        return count;
    }

    [[nodiscard]] constexpr u32 leading_zeros() const noexcept {
        const U value = bits();
        u32 count = 0;

        for (u32 i = BITS; i != 0; --i) {
            if (((value >> (i - 1)) & U{1}) != 0) {
                break;
            }
            ++count;
        }

        return count;
    }

    [[nodiscard]] constexpr Saturating rotate_left(u32 n) const noexcept {
        n %= BITS;
        if (n == 0) {
            return *this;
        }

        const U value = bits();
        return Saturating{from_bits(static_cast<U>((value << n) | (value >> (BITS - n))))};
    }

    [[nodiscard]] constexpr Saturating rotate_right(u32 n) const noexcept {
        n %= BITS;
        if (n == 0) {
            return *this;
        }

        const U value = bits();
        return Saturating{from_bits(static_cast<U>((value >> n) | (value << (BITS - n))))};
    }

    [[nodiscard]] constexpr Saturating reverse_bits() const noexcept {
        U value = bits();
        U result = 0;

        for (u32 i = 0; i < BITS; ++i) {
            result = static_cast<U>((result << 1) | (value & U{1}));
            value >>= 1;
        }

        return Saturating{from_bits(result)};
    }

    [[nodiscard]] constexpr Saturating swap_bytes() const noexcept {
        if constexpr (sizeof(T) == 1) {
            return *this;
        } else {
            U value = bits();
            U result = 0;

            for (u32 i = 0; i < sizeof(T); ++i) {
                result = static_cast<U>((result << 8) | (value & U{0xFF}));
                value >>= 8;
            }

            return Saturating{from_bits(result)};
        }
    }

    [[nodiscard]] constexpr Saturating to_be() const noexcept {
        return target::endian::LITTLE ? swap_bytes() : *this;
    }

    [[nodiscard]] constexpr Saturating to_le() const noexcept {
        return target::endian::LITTLE ? *this : swap_bytes();
    }

    [[nodiscard]] static constexpr Saturating from_be(Saturating value) noexcept {
        return value.to_be();
    }

    [[nodiscard]] static constexpr Saturating from_le(Saturating value) noexcept {
        return value.to_le();
    }

    [[nodiscard]] friend constexpr bool operator==(Saturating a, Saturating b) noexcept {
        return a._inner == b._inner;
    }

    [[nodiscard]] friend constexpr bool operator!=(Saturating a, Saturating b) noexcept {
        return !(a == b);
    }

    [[nodiscard]] friend constexpr bool operator<(Saturating a, Saturating b) noexcept {
        return a._inner < b._inner;
    }

    [[nodiscard]] friend constexpr bool operator<=(Saturating a, Saturating b) noexcept {
        return !(b < a);
    }

    [[nodiscard]] friend constexpr bool operator>(Saturating a, Saturating b) noexcept {
        return b < a;
    }

    [[nodiscard]] friend constexpr bool operator>=(Saturating a, Saturating b) noexcept {
        return !(a < b);
    }

private:
    [[nodiscard]] static constexpr T from_bits(U bits) noexcept {
        if constexpr (IsUnsigned<T>{}) {
            return static_cast<T>(bits);
        } else {
            constexpr U sign = static_cast<U>(U{1} << (BITS - 1));

            if ((bits & sign) == 0) {
                return static_cast<T>(bits);
            }

            const U magnitude = static_cast<U>(~bits + U{1});
            return magnitude == sign
                ? std::numeric_limits<T>::min()
                : static_cast<T>(-static_cast<T>(magnitude));
        }
    }

    [[nodiscard]] constexpr U bits() const noexcept { return static_cast<U>(_inner); }
};

} // namespace oxide

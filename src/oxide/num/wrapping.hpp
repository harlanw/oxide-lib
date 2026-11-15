#pragma once

#include <climits>
#include <limits>
#include <type_traits>

#include <oxide/compiler/target.hpp>
#include <oxide/num/types.hpp>
#include <oxide/assert.hpp>
#include <oxide/traits.hpp>

namespace oxide {

template <typename T, IfIntegral<T> = 0>
class Wrapping final {
    static_assert(IsSame<T, RemoveCv<T>>{}, "T must be unqualified");
    static_assert(CHAR_BIT == 8, "Wrapping requires 8-bit bytes");

    using U = std::make_unsigned_t<T>;
    using Wide = Conditional<(sizeof(U) < sizeof(unsigned)), unsigned, U>;

    static_assert(
        std::numeric_limits<U>::digits == sizeof(T) * CHAR_BIT,
        "Wrapping requires integers without padding bits"
    );
    static_assert(
        !IsSigned<T>{} || std::numeric_limits<T>::min() == -std::numeric_limits<T>::max() - 1,
        "Wrapping requires two's-complement signed integers"
    );

    T _inner{0};

public:
    static constexpr u32 BITS = std::numeric_limits<U>::digits;
    static constexpr T MIN = std::numeric_limits<T>::min();
    static constexpr T MAX = std::numeric_limits<T>::max();

    constexpr Wrapping() noexcept = default;
    explicit constexpr Wrapping(T value) noexcept : _inner(value) {}

    [[nodiscard]] constexpr T get() const noexcept { return _inner; }

    [[nodiscard]] friend constexpr Wrapping operator+(Wrapping a, Wrapping b) noexcept {
        return Wrapping{
            from_bits(static_cast<U>(static_cast<Wide>(a.bits()) + static_cast<Wide>(b.bits()))),
        };
    }

    [[nodiscard]] friend constexpr Wrapping operator-(Wrapping a, Wrapping b) noexcept {
        return Wrapping{
            from_bits(static_cast<U>(static_cast<Wide>(a.bits()) - static_cast<Wide>(b.bits()))),
        };
    }

    [[nodiscard]] friend constexpr Wrapping operator*(Wrapping a, Wrapping b) noexcept {
        return Wrapping{
            from_bits(static_cast<U>(static_cast<Wide>(a.bits()) * static_cast<Wide>(b.bits()))),
        };
    }

    [[nodiscard]] friend constexpr Wrapping operator/(Wrapping a, Wrapping b) noexcept {
        OXIDE_ASSERT(b._inner != 0);

        if constexpr (IsSigned<T>{}) {
            if (a._inner == MIN && b._inner == T{-1}) {
                return Wrapping{MIN};
            }
        }

        return Wrapping{static_cast<T>(a._inner / b._inner)};
    }

    [[nodiscard]] friend constexpr Wrapping operator%(Wrapping a, Wrapping b) noexcept {
        OXIDE_ASSERT(b._inner != 0);

        if constexpr (IsSigned<T>{}) {
            if (a._inner == MIN && b._inner == T{-1}) {
                return Wrapping{T{0}};
            }
        }

        return Wrapping{static_cast<T>(a._inner % b._inner)};
    }

    [[nodiscard]] constexpr Wrapping operator-() const noexcept { return Wrapping{T{0}} - *this; }

    constexpr Wrapping& operator+=(Wrapping other) noexcept { return *this = *this + other; }
    constexpr Wrapping& operator-=(Wrapping other) noexcept { return *this = *this - other; }
    constexpr Wrapping& operator*=(Wrapping other) noexcept { return *this = *this * other; }
    constexpr Wrapping& operator/=(Wrapping other) noexcept { return *this = *this / other; }
    constexpr Wrapping& operator%=(Wrapping other) noexcept { return *this = *this % other; }

    constexpr Wrapping& operator+=(T other) noexcept { return *this += Wrapping{other}; }
    constexpr Wrapping& operator-=(T other) noexcept { return *this -= Wrapping{other}; }
    constexpr Wrapping& operator*=(T other) noexcept { return *this *= Wrapping{other}; }
    constexpr Wrapping& operator/=(T other) noexcept { return *this /= Wrapping{other}; }
    constexpr Wrapping& operator%=(T other) noexcept { return *this %= Wrapping{other}; }

    [[nodiscard]] friend constexpr Wrapping operator&(Wrapping a, Wrapping b) noexcept {
        return Wrapping{from_bits(static_cast<U>(a.bits() & b.bits()))};
    }

    [[nodiscard]] friend constexpr Wrapping operator|(Wrapping a, Wrapping b) noexcept {
        return Wrapping{from_bits(static_cast<U>(a.bits() | b.bits()))};
    }

    [[nodiscard]] friend constexpr Wrapping operator^(Wrapping a, Wrapping b) noexcept {
        return Wrapping{from_bits(static_cast<U>(a.bits() ^ b.bits()))};
    }

    [[nodiscard]] constexpr Wrapping operator~() const noexcept {
        return Wrapping{from_bits(static_cast<U>(~bits()))};
    }

    constexpr Wrapping& operator&=(Wrapping other) noexcept { return *this = *this & other; }
    constexpr Wrapping& operator|=(Wrapping other) noexcept { return *this = *this | other; }
    constexpr Wrapping& operator^=(Wrapping other) noexcept { return *this = *this ^ other; }

    constexpr Wrapping& operator&=(T other) noexcept { return *this &= Wrapping{other}; }
    constexpr Wrapping& operator|=(T other) noexcept { return *this |= Wrapping{other}; }
    constexpr Wrapping& operator^=(T other) noexcept { return *this ^= Wrapping{other}; }

    [[nodiscard]] constexpr Wrapping wrapping_shl(u32 n) const noexcept {
        n %= BITS;
        return Wrapping{from_bits(static_cast<U>(static_cast<Wide>(bits()) << n))};
    }

    [[nodiscard]] constexpr Wrapping wrapping_shr(u32 n) const noexcept {
        n %= BITS;
        if (n == 0) {
            return *this;
        }

        Wide value = static_cast<Wide>(bits()) >> n;

        if constexpr (IsSigned<T>{}) {
            if (_inner < 0) {
                value |= static_cast<Wide>(std::numeric_limits<U>::max()) << (BITS - n);
            }
        }

        return Wrapping{from_bits(static_cast<U>(value))};
    }

    template <typename Shift, IfIntegral<Shift> = 0>
    [[nodiscard]] friend constexpr Wrapping operator<<(Wrapping value, Shift count) noexcept {
        static_assert(!IsSame<Shift, bool>{}, "boolean shift count is not supported");

        if constexpr (IsSigned<Shift>{}) {
            if (count < 0) {
                using US = std::make_unsigned_t<Shift>;
                const US magnitude = static_cast<US>(US{0} - static_cast<US>(count));
                return value.wrapping_shr(static_cast<u32>(magnitude));
            }
        }

        return value.wrapping_shl(static_cast<u32>(count));
    }

    template <typename Shift, IfIntegral<Shift> = 0>
    [[nodiscard]] friend constexpr Wrapping operator>>(Wrapping value, Shift count) noexcept {
        static_assert(!IsSame<Shift, bool>{}, "boolean shift count is not supported");

        if constexpr (IsSigned<Shift>{}) {
            if (count < 0) {
                using US = std::make_unsigned_t<Shift>;
                const US magnitude = static_cast<US>(US{0} - static_cast<US>(count));
                return value.wrapping_shl(static_cast<u32>(magnitude));
            }
        }

        return value.wrapping_shr(static_cast<u32>(count));
    }

    template <typename Shift, IfIntegral<Shift> = 0>
    constexpr Wrapping& operator<<=(Shift n) noexcept {
        return *this = *this << n;
    }

    template <typename Shift, IfIntegral<Shift> = 0>
    constexpr Wrapping& operator>>=(Shift n) noexcept {
        return *this = *this >> n;
    }

    [[nodiscard]] constexpr Wrapping pow(u32 exp) const noexcept {
        Wrapping result{T{1}};
        Wrapping base{*this};

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

    [[nodiscard]] constexpr Wrapping abs() const noexcept {
        static_assert(IsSigned<T>{}, "abs requires signed T");
        return _inner < 0 ? -(*this) : *this;
    }

    [[nodiscard]] constexpr Wrapping signum() const noexcept {
        static_assert(IsSigned<T>{}, "signum requires signed T");
        return Wrapping{static_cast<T>((_inner > 0) - (_inner < 0))};
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

    [[nodiscard]] constexpr u32 trailing_zeros() const noexcept {
        U value = bits();
        if (value == 0) {
            return BITS;
        }

        u32 count = 0;

        while ((value & U{1}) == 0) {
            value = static_cast<U>(value >> 1);
            ++count;
        }

        return count;
    }

    [[nodiscard]] constexpr Wrapping rotate_left(u32 n) const noexcept {
        n %= BITS;
        if (n == 0) {
            return *this;
        }

        const Wide value = bits();
        return Wrapping{from_bits(static_cast<U>((value << n) | (value >> (BITS - n))))};
    }

    [[nodiscard]] constexpr Wrapping rotate_right(u32 n) const noexcept {
        n %= BITS;
        if (n == 0) {
            return *this;
        }

        const Wide value = bits();
        return Wrapping{from_bits(static_cast<U>((value >> n) | (value << (BITS - n))))};
    }

    [[nodiscard]] constexpr Wrapping reverse_bits() const noexcept {
        U value = bits();
        U result = 0;

        for (u32 i = 0; i < BITS; ++i) {
            result = static_cast<U>((static_cast<Wide>(result) << 1) | (value & U{1}));
            value = static_cast<U>(value >> 1);
        }

        return Wrapping{from_bits(result)};
    }

    [[nodiscard]] constexpr Wrapping swap_bytes() const noexcept {
        U value = bits();
        U result = 0;

        for (u32 i = 0; i < sizeof(T); ++i) {
            result = static_cast<U>((static_cast<Wide>(result) << 8) | (value & U{0xff}));
            value = static_cast<U>(value >> 8);
        }

        return Wrapping{from_bits(result)};
    }

    [[nodiscard]] constexpr Wrapping to_be() const noexcept {
        return target::endian::LITTLE ? swap_bytes() : *this;
    }

    [[nodiscard]] constexpr Wrapping to_le() const noexcept {
        return target::endian::LITTLE ? swap_bytes() : *this;
    }

    [[nodiscard]] static constexpr Wrapping from_be(Wrapping value) noexcept {
        return value.to_be();
    }

    [[nodiscard]] static constexpr Wrapping from_le(Wrapping value) noexcept {
        return value.to_le();
    }

    [[nodiscard]] friend constexpr bool operator==(Wrapping a, Wrapping b) noexcept {
        return a._inner == b._inner;
    }

    [[nodiscard]] friend constexpr bool operator!=(Wrapping a, Wrapping b) noexcept {
        return !(a == b);
    }

    [[nodiscard]] friend constexpr bool operator<(Wrapping a, Wrapping b) noexcept {
        return a._inner < b._inner;
    }

    [[nodiscard]] friend constexpr bool operator<=(Wrapping a, Wrapping b) noexcept {
        return !(b < a);
    }

    [[nodiscard]] friend constexpr bool operator>(Wrapping a, Wrapping b) noexcept { return b < a; }

    [[nodiscard]] friend constexpr bool operator>=(Wrapping a, Wrapping b) noexcept {
        return !(a < b);
    }

private:
    [[nodiscard]] static constexpr T from_bits(U value) noexcept {
        if constexpr (IsUnsigned<T>{}) {
            return static_cast<T>(value);
        } else {
            if (value <= static_cast<U>(MAX)) {
                return static_cast<T>(value);
            }
            return static_cast<T>(-1 - static_cast<T>(static_cast<U>(~value)));
        }
    }

    [[nodiscard]] constexpr U bits() const noexcept { return static_cast<U>(_inner); }
};

} // namespace oxide

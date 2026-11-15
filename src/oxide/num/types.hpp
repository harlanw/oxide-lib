#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace oxide {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;
using isize = std::ptrdiff_t;

using uptr = std::uintptr_t;
using iptr = std::intptr_t;

template <typename T>
struct NumericInfo {
    using Type = T;

    static constexpr Type MIN = std::numeric_limits<T>::lowest();
    static constexpr Type MAX = std::numeric_limits<T>::max();
    static constexpr u32 BITS = sizeof(T) * 8;
};

using U8 = NumericInfo<u8>;
using U16 = NumericInfo<u16>;
using U32 = NumericInfo<u32>;
using U64 = NumericInfo<u64>;

using I8 = NumericInfo<i8>;
using I16 = NumericInfo<i16>;
using I32 = NumericInfo<i32>;
using I64 = NumericInfo<i64>;

using F32 = NumericInfo<f32>;
using F64 = NumericInfo<f64>;

using Usize = NumericInfo<usize>;
using Isize = NumericInfo<isize>;

static_assert(
    sizeof(f32) == 4 && std::numeric_limits<f32>::is_iec559,
    "Oxide requires IEEE 754 binary32"
);

static_assert(
    sizeof(f64) == 8 && std::numeric_limits<f64>::is_iec559,
    "Oxide requires IEEE 754 binary64"
);

static_assert(sizeof(usize) == sizeof(void*));
static_assert(sizeof(isize) == sizeof(void*));

} // namespace oxide

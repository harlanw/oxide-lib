#pragma once

#include <initializer_list>

#include <oxide/compiler/location.hpp>
#include <oxide/compiler/macros.hpp>
#include <oxide/compiler/target.hpp>

// clang-format off

#if OXIDE_HAS_BUILTIN(__builtin_expect)
    #define OXIDE_BUILTIN_EXPECT(x, expected) __builtin_expect((x), (expected))
#else
    #define OXIDE_BUILTIN_EXPECT(x, expected) (x)
#endif

#define OXIDE_BUILTIN_LIKELY(x)   OXIDE_BUILTIN_EXPECT(!!(x), 1)
#define OXIDE_BUILTIN_UNLIKELY(x) OXIDE_BUILTIN_EXPECT(!!(x), 0)

#if defined(OXIDE_COMPILER_CLANG) || defined(OXIDE_COMPILER_GCC)
    #define OXIDE_COLD [[gnu::cold]]
    #define OXIDE_HOT  [[gnu::hot]]
#else
    #define OXIDE_COLD
    #define OXIDE_HOT
#endif

#if OXIDE_HAS_ATTRIBUTE(always_inline)
    #define OXIDE_ALWAYS_INLINE inline __attribute__((always_inline))
    #define OXIDE_NOINLINE __attribute__((noinline))
#elif OXIDE_TARGET_ENV_MSVC
    #define OXIDE_ALWAYS_INLINE __forceinline
    #define OXIDE_NOINLINE __declspec(noinline)
#else
    #define OXIDE_ALWAYS_INLINE inline
    #define OXIDE_NOINLINE
#endif

#if OXIDE_HAS_BUILTIN(__builtin_assume)
    #define OXIDE_BUILTIN_ASSUME(cond) __builtin_assume(cond)
#elif OXIDE_TARGET_ENV_MSVC
    #define OXIDE_BUILTIN_ASSUME(cond) __assume(cond)
#elif defined(__GNUC__) && __GNUC__ >= 13
    #define OXIDE_BUILTIN_ASSUME(cond) [[gnu::assume(cond)]]
#else
    #error "Oxide: OXIDE_BUILTIN_ASSUME requires compiler intrinsic support"
#endif

#if OXIDE_HAS_BUILTIN(__builtin_unreachable)
    #define OXIDE_BUILTIN_UNREACHABLE() __builtin_unreachable()
#elif OXIDE_TARGET_ENV_MSVC
    #define OXIDE_BUILTIN_UNREACHABLE() __assume(false)
#else
    #error "Oxide: OXIDE_BUILTIN_UNREACHABLE requires compiler intrinsic support"
#endif

#define OXIDE_DISCARD(expr) ::oxide::discard(expr)

// clang-format on

namespace oxide::hints {

[[noreturn]] OXIDE_ALWAYS_INLINE void unreachable_unchecked() noexcept {
    OXIDE_BUILTIN_UNREACHABLE();
}

OXIDE_ALWAYS_INLINE void assume(bool cond) noexcept {
    OXIDE_BUILTIN_ASSUME(cond);
}

OXIDE_ALWAYS_INLINE constexpr bool likely(bool cond) noexcept {
    return static_cast<bool>(OXIDE_BUILTIN_LIKELY(cond));
}

OXIDE_ALWAYS_INLINE constexpr bool unlikely(bool cond) noexcept {
    return static_cast<bool>(OXIDE_BUILTIN_UNLIKELY(cond));
}

template <class... Args>
constexpr void discard(Args&&...) noexcept {}

} // namespace oxide::hints

#pragma once

#include <oxide/compiler/macros.hpp>
#include <oxide/compiler/target.hpp>

#define OXIDE_CONCAT_IMPL(a, b) a##b
#define OXIDE_CONCAT(a, b) OXIDE_CONCAT_IMPL(a, b)

// Stable within a macro expansion; generated locals must have their own scope.
#define OXIDE_MANGLE_NAME(name) OXIDE_CONCAT(name, __LINE__)

#define OXIDE_DISABLE_COPY(T) \
    T(const T&) = delete; \
    T& operator=(const T&) = delete;

#define OX_DISABLE_MOVE(T) \
    T(T&&) = delete; \
    T& operator=(T&&) = delete;

// clang-format off
#define OXIDE_PRAGMA(x) _Pragma(#x)

#if OXIDE_COMPILER_CLANG
    #define OXIDE_DIAGNOSTIC_PUSH \
        OXIDE_PRAGMA(clang diagnostic push)
    #define OXIDE_DIAGNOSTIC_POP \
        OXIDE_PRAGMA(clang diagnostic pop)
    #define OXIDE_DIAGNOSTIC_IGNORE(w) \
        OXIDE_PRAGMA(clang diagnostic ignored w)

#elif OXIDE_COMPILER_GCC
    #define OXIDE_DIAGNOSTIC_PUSH \
        OXIDE_PRAGMA(GCC diagnostic push)
    #define OXIDE_DIAGNOSTIC_POP \
        OXIDE_PRAGMA(GCC diagnostic pop)
    #define OXIDE_DIAGNOSTIC_IGNORE(w) \
        OXIDE_PRAGMA(GCC diagnostic ignored w)

#else
    #define OXIDE_DIAGNOSTIC_PUSH
    #define OXIDE_DIAGNOSTIC_POP
    #define OXIDE_DIAGNOSTIC_IGNORE(w)
#endif

#define OXIDE_SILENCE_DANGLING_ELSE_BEGIN \
    OXIDE_DIAGNOSTIC_PUSH                 \
    OXIDE_DIAGNOSTIC_IGNORE("-Wdangling-else")

#define OXIDE_SILENCE_DANGLING_ELSE_END \
    OXIDE_DIAGNOSTIC_POP

// clang-format on

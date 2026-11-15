#pragma once

#include <cstdint>

#include <oxide/compiler/macros.hpp>

#if !OXIDE_HAS_BUILTIN(__builtin_FILE) \
    || !OXIDE_HAS_BUILTIN(__builtin_FUNCTION) \
    || !OXIDE_HAS_BUILTIN(__builtin_LINE)
#error "Oxide: SourceLocation requires compiler source-location builtins"
#endif

namespace oxide {

struct SourceLocation {
    const char* file = "unknown";
    const char* function = "unknown";
    std::uint_least32_t lineno{0};
    std::uint_least32_t colno{0};

    constexpr const char* file_name() const noexcept { return file; }

    constexpr const char* function_name() const noexcept { return function; }

    constexpr std::uint_least32_t line() const noexcept { return lineno; }

    constexpr std::uint_least32_t column() const noexcept { return colno; }

    static constexpr SourceLocation current(
        const char* file = __builtin_FILE(),
        const char* function = __builtin_FUNCTION(),
        std::uint_least32_t line = __builtin_LINE(),
#if OXIDE_HAS_BUILTIN(__builtin_COLUMN)
        std::uint_least32_t column = __builtin_COLUMN()
#else
        std::uint_least32_t column = 0
#endif
    ) noexcept {
        return {file, function, line, column};
    }
};

} // namespace oxide

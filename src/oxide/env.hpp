#pragma once

#include <limits.h>
#include <unistd.h>

#include <array>
#include <cstdlib>
#include <cstring>

#include <oxide/compiler/target.hpp>
#include <oxide/num/types.hpp>

namespace oxide::env {

inline constexpr usize MAX_PATH_LEN = PATH_MAX;

inline const char* var(const char* key) noexcept {
    return std::getenv(key);
}

[[nodiscard]] inline const char* tmp() noexcept {
    static const auto path = [] {
        constexpr char FALLBACK[] = "/tmp";
        static_assert(MAX_PATH_LEN >= sizeof(FALLBACK));
        std::array<char, MAX_PATH_LEN> result{};

        const auto copy = [&](const char* src) noexcept {
            if (!src || !*src) {
                return false;
            }

            const usize len = ::strnlen(src, result.size());
            if (len == result.size()) {
                return false;
            }

            std::memcpy(result.data(), src, len + 1);
            return true;
        };

        // Respect the user's configured temporary directory.
        if (copy(std::getenv("TMPDIR"))) {
            return result;
        }

        // clang-format off
        #if OXIDE_TARGET_OS_MACOS
            // macOS provides a per-user temporary directory.
            const usize size = ::confstr(_CS_DARWIN_USER_TEMP_DIR, result.data(), result.size());

            if (size > 1 && size <= result.size()) {
                return result;
            }
        #endif // clang-format on

        std::memcpy(result.data(), FALLBACK, sizeof(FALLBACK));
        return result;
    }();

    return path.data();
}

} // namespace oxide::env

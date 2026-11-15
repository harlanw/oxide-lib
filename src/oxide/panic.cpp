#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <cstring>

#include <oxide/assert.hpp>
#include <oxide/io.hpp>

namespace oxide::detail {
namespace {

struct PanicBuffer final {
    static constexpr usize CAPACITY = 1024;
    static constexpr usize MAX_CONTENT = CAPACITY - (sizeof("...\n") - 1);

    char data[CAPACITY];
    usize len{0};
    bool truncated{false};

    OXIDE_ALWAYS_INLINE void append(const char* str, usize size) noexcept {
        const usize available = MAX_CONTENT - len;
        const usize count = std::min(size, available);

        if (count != 0) {
            std::memcpy(data + len, str, count);
            len += count;
        }

        truncated |= count != size;
    }

    OXIDE_ALWAYS_INLINE void append(Str str) noexcept { append(str.data(), str.len()); }

    OXIDE_ALWAYS_INLINE void append(const char* str) noexcept {
        if (str != nullptr) {
            append(str, std::strlen(str));
        }
    }

    OXIDE_ALWAYS_INLINE void append_number(std::uint_least32_t value) noexcept {
        char digits[32];
        const auto result = std::to_chars(digits, digits + sizeof(digits), value);

        if (result.ec == std::errc{}) {
            append(digits, static_cast<usize>(result.ptr - digits));
        }
    }

    [[nodiscard]] OXIDE_ALWAYS_INLINE usize finish() noexcept {
        if (truncated) {
            std::memcpy(data + len, "...\n", 4);
            return len + 4;
        }

        data[len] = '\n';
        return len + 1;
    }
};

[[noreturn]] OXIDE_COLD void
report_failure(const SourceLocation& location, Str base, Str extra, Str expression) noexcept {
    PanicBuffer buffer;

    const bool use_color = io::is_terminal();

    // Location
    if (use_color) {
        buffer.append("\x1b[90m");
    }
    buffer.append(location.file_name());
    buffer.append(":");
    buffer.append_number(location.line());
    buffer.append(": ");
    if (use_color) {
        buffer.append("\x1b[0m");
    }

    // Error
    if (use_color) {
        buffer.append("\x1b[1;31m");
    }
    buffer.append("error:");
    if (use_color) {
        buffer.append("\x1b[0m");
    }

    buffer.append(" ");
    buffer.append(base);

    // Details
    if (!expression.is_empty()) {
        buffer.append("\n  expression: ");
        buffer.append(expression);
    }

    if (!extra.is_empty()) {
        buffer.append("\n  message: ");
        buffer.append(extra);
    }

    const usize len = buffer.finish();

    // Handle interrupted and partial writes.
    usize written = 0;
    while (written < len) {
        const ssize_t n = ::write(STDERR_FILENO, buffer.data + written, len - written);

        if (n > 0) {
            written += static_cast<usize>(n);
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    std::abort();
}

} // namespace

[[noreturn]] OXIDE_COLD void
fail_fast(const SourceLocation& location, Str base, Str extra) noexcept {
    report_failure(location, base, extra, "");
}

[[noreturn]] OXIDE_COLD void
assertion_failed(const SourceLocation& location, Str expression, Str message) noexcept {
    report_failure(location, "assertion failed", message, expression);
}

} // namespace oxide::detail

#pragma once

#include <oxide/core.hpp>

namespace oxide::posix::darwin {

// Darwin's close() is a pthread cancellation point. Apple exposes this
// non-cancelable variant for cases where cancellation during resource cleanup
// would be unsafe.
extern "C" i32 close_nocancel(i32 fd) __asm__("_close$NOCANCEL");

[[nodiscard]] inline i32 close(RawFd fd) noexcept {
    return close_nocancel(fd.inner);
}

} // namespace oxide::posix::darwin

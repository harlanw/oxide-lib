#pragma once

#include <unistd.h>

#include <oxide/types.hpp>
#include <oxide/posix/pal/errno.hpp>
#include <oxide/posix/pal/fd.hpp>

#if OXIDE_TARGET_OS_MACOS // clang-format off
    #include <oxide/posix/darwin/unistd.hpp>
#elif OXIDE_TARGET_OS_LINUX
    #include <sys/syscall.h>
#endif // clang-format on

namespace oxide::posix {

inline i32 close(RawFd fd) noexcept {
    const auto previous_errno = Errno::last(); // clang-format off
    
    #if OXIDE_TARGET_OS_MACOS
        // Normal Darwin close() can return EINTR without having closed the
        // descriptor when pthread cancellation is involved. close$NOCANCEL
        // bypasses the cancellation-point behavior.
        const i32 result = darwin::close(fd);
    #elif OXIDE_TARGET_OS_LINUX
        // glibc close() is a pthread cancellation point. Go directly to the
        // kernel so cancellation cannot unwind the thread during cleanup.
        //
        // Never retry after EINTR. The kernel may already have released the
        // descriptor and another thread may have reused the same integer fd.
        const i32 result = as<i32>(::syscall(SYS_close, fd.inner));
    #endif // clang-format on

    if (result == 0) {
        return 0;
    }

    if (Errno::last().interrupted() || Errno::last().in_progress()) {
        // The descriptor is considered consumed. Since we're converting this
        // condition into success, don't leak EINTR/EINPROGRESS through errno.
        return WITH_ERRNO(previous_errno, 0);
    }

    return -1;
}

inline i32 ftruncate(RawFd fd, usize size) noexcept {
    return ::ftruncate(fd, static_cast<off_t>(size));
}

[[nodiscard]] inline ssize_t read(RawFd fd, void* buffer, size_t size) noexcept {
    return ::read(fd, buffer, size);
}

inline i32 unlink(CStr path) noexcept {
    return ::unlink(path.data());
}

} // namespace oxide::posix

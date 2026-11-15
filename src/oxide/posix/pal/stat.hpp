#pragma once

#include <sys/stat.h>

#include <oxide/types.hpp>
#include <oxide/posix/pal/fcntl.hpp>
#include <oxide/posix/pal/fd.hpp>

namespace oxide::posix {

inline i32 fchmod(RawFd fd, Mode mode) noexcept {
    return ::fchmod(fd, mode.bits());
}

inline i32 fstat(RawFd fd, struct stat* st) noexcept {
    return ::fstat(fd, st);
}

} // namespace oxide::posix

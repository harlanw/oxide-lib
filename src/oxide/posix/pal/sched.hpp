#pragma once

#include <sched.h>

#include <oxide/num/types.hpp>

namespace oxide::posix {

inline i32 sched_yield() noexcept {
    return ::sched_yield();
}

} // namespace oxide::posix

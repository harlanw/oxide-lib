#pragma once

#include <semaphore.h>

#include <chrono>

#include <oxide/types.hpp>

namespace oxide::posix {

[[nodiscard]] bool sem_wait(sem_t*);

[[nodiscard]] Option<bool> sem_trywait(sem_t*);

[[nodiscard]] Option<bool> sem_waitfor(sem_t*, std::chrono::nanoseconds);

} // namespace oxide::posix

#pragma once

#include <semaphore.h>

#include <oxide/num.hpp>

#include <oxide/posix/common/semaphore.hpp>

namespace oxide::posix {

namespace linux_backend {
class SemHandle;
}

using UnnamedSemHandle = linux_backend::SemHandle;

i32 sem_init(UnnamedSemHandle&, bool, u32);
i32 sem_destroy(UnnamedSemHandle&);

namespace linux_backend {

/// In-place storage for an unnamed POSIX semaphore.
///
/// Copy SemRef instead of copying or moving this state.
class SemHandle final {
public:
    SemHandle() noexcept = default;

    SemHandle(const SemHandle&) = delete;
    SemHandle& operator=(const SemHandle&) = delete;

    SemHandle(SemHandle&&) = delete;
    SemHandle& operator=(SemHandle&&) = delete;

private:
    sem_t _handle{};

    friend class SemRef;

    friend i32 posix::sem_init(UnnamedSemHandle&, bool, u32);
    friend i32 posix::sem_destroy(UnnamedSemHandle&);
};

/// Copyable, non-owning reference to an unnamed POSIX semaphore.
class SemRef final {
public:
    explicit SemRef(SemHandle& state) noexcept : _handle(&state._handle) {}

    SemRef(const SemRef&) noexcept = default;
    SemRef& operator=(const SemRef&) noexcept = default;

    SemRef(SemRef&&) noexcept = default;
    SemRef& operator=(SemRef&&) noexcept = default;

    [[nodiscard]] bool post() const { return ::sem_post(_handle) == 0; }

    [[nodiscard]] bool wait() const { return posix::sem_wait(_handle); }

    [[nodiscard]] Option<bool> try_wait() const { return posix::sem_trywait(_handle); }

    [[nodiscard]] Option<bool> wait_for(std::chrono::nanoseconds timeout) const {
        return posix::sem_waitfor(_handle, timeout);
    }

private:
    sem_t* _handle;
};

static_assert(oxide::IsTriviallyCopyable<SemRef>{});

} // namespace linux_backend

} // namespace oxide::posix

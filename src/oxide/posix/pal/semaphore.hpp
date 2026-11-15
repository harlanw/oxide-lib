#pragma once

#include <semaphore.h>

#include <chrono>
#include <utility>

#include <oxide/compiler/target.hpp>
#include <oxide/types.hpp>
#include <oxide/posix/ipc.hpp>
#include <oxide/posix/pal/errno.hpp>
#include <oxide/posix/pal/fcntl.hpp>
#include <oxide/posix/pal/sched.hpp>

#include <oxide/posix/common/semaphore.hpp>
#if OXIDE_TARGET_OS_MACOS // clang-format off
    #include <oxide/posix/darwin/semaphore.hpp>
#elif OXIDE_TARGET_OS_LINUX
    #include <oxide/posix/linux/semaphore.hpp>
#endif // clang-format on

namespace oxide::posix {

namespace detail {
class SemHandle;
} // namespace detail

#if OXIDE_TARGET_OS_MACOS // clang-format off
    using NamedSemHandle = detail::SemHandle;
    using UnnamedSemHandle = darwin::SemHandle;
    using UnnamedSemRef = darwin::SemRef;
#elif OXIDE_TARGET_OS_LINUX
    using NamedSemHandle = detail::SemHandle;
    using UnnamedSemHandle = linux_backend::SemHandle;
    using UnnamedSemRef = linux_backend::SemRef;
#endif // clang-format on

i32 sem_close(const NamedSemHandle&);
Option<NamedSemHandle> sem_open(const CStr&, CreationPolicy);
Option<NamedSemHandle> sem_open(const CStr&, CreationPolicy, Mode, u32);
i32 sem_post(const NamedSemHandle&);
i32 sem_wait(const NamedSemHandle&);
i32 sem_trywait(const NamedSemHandle&);
i32 sem_unlink(const CStr&);

i32 sem_init(UnnamedSemHandle&, bool, u32);
i32 sem_destroy(UnnamedSemHandle&);

namespace detail {

/// Non-owning reference to a named POSIX semaphore.
///
/// The referenced semaphore must remain valid for the lifetime of the reference.
class SemHandle final {
public:
    SemHandle(const SemHandle& other) : _handle(other._handle) {}
    SemHandle& operator=(const SemHandle& other) {
        if (this != &other) {
            _handle = other._handle;
        }

        return *this;
    }

    SemHandle(SemHandle&& other) noexcept : _handle(std::exchange(other._handle, SEM_FAILED)) {}
    SemHandle& operator=(SemHandle&& other) noexcept {
        if (this != &other) {
            _handle = std::exchange(other._handle, SEM_FAILED);
        }

        return *this;
    }

    [[nodiscard]] bool post() const { return ::sem_post(_handle) == 0; }

    [[nodiscard]] bool wait() const { return posix::sem_wait(_handle); }

    [[nodiscard]] Option<bool> try_wait() const { return posix::sem_trywait(_handle); }

    [[nodiscard]] Option<bool> wait_for(std::chrono::nanoseconds timeout) const {
        return posix::sem_waitfor(_handle, timeout);
    }

    [[nodiscard]] bool is_failed() const { return _handle == SEM_FAILED; }

private:
    sem_t* _handle{SEM_FAILED};

    explicit SemHandle(sem_t& handle) : _handle(&handle) {}

    [[nodiscard]] sem_t* get() const noexcept { return _handle; }

    friend i32 posix::sem_close(const NamedSemHandle&);
    friend Option<NamedSemHandle> posix::sem_open(const CStr&, CreationPolicy);
    friend Option<NamedSemHandle> posix::sem_open(const CStr&, CreationPolicy, Mode, u32);
    friend i32 posix::sem_post(const NamedSemHandle&);
    friend i32 posix::sem_trywait(const NamedSemHandle&);
    friend i32 posix::sem_wait(const NamedSemHandle&);
};

} // namespace detail

inline i32 sem_close(const NamedSemHandle& handle) {
    return ::sem_close(handle.get());
}

inline Option<NamedSemHandle> sem_open(const CStr& name, CreationPolicy policy) {
    auto* handle = ::sem_open(name.data(), policy.oflags().bits());
    return handle == SEM_FAILED ? NONE : some(NamedSemHandle(*handle));
}

inline Option<NamedSemHandle>
sem_open(const CStr& name, CreationPolicy policy, Mode mode, u32 value) {
    auto* handle = ::sem_open(name.data(), policy.oflags().bits(), mode.bits(), value);
    return handle == SEM_FAILED ? NONE : some(NamedSemHandle(*handle));
}

inline i32 sem_post(const NamedSemHandle& handle) {
    return ::sem_post(handle.get());
}

inline i32 sem_trywait(const NamedSemHandle& handle) {
    return ::sem_trywait(handle.get());
}

inline i32 sem_unlink(const CStr& name) {
    return ::sem_unlink(name.data());
}

inline i32 sem_wait(const NamedSemHandle& handle) {
    return ::sem_wait(handle.get());
}

inline i32 sem_init(UnnamedSemHandle& state, bool shared, u32 init_value) { // clang-format off
    #if OXIDE_TARGET_OS_MACOS
        return darwin::semaphore_init(state, shared, init_value);
    #elif OXIDE_TARGET_OS_LINUX
        return ::sem_init(&state._handle, shared, init_value);
    #endif // clang-format on
}

inline i32 sem_destroy(UnnamedSemHandle& state) { // clang-format off
    #if OXIDE_TARGET_OS_MACOS
        return darwin::semaphore_destroy(state);
    #elif OXIDE_TARGET_OS_LINUX
        return ::sem_destroy(&state._handle);
    #endif // clang-format on
}

} // namespace oxide::posix

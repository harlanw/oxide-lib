#pragma once

#pragma once

#include <semaphore.h>
#include <time.h>

#include <os/os_sync_wait_on_address.h>

#include <atomic>
#include <chrono>
#include <type_traits>

#include <oxide/core.hpp>
#include <oxide/posix/pal/errno.hpp>
#include <oxide/sync/atomic.hpp>

// clang-format off
#if defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) \
    && __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 140400
    #error "Darwin unnamed semaphores require macOS 14.4 or newer"
#endif // clang-format on

namespace oxide::posix::darwin {

class SemHandle;
class SemRef;

i32 semaphore_init(SemHandle&, bool, u32);
i32 semaphore_destroy(SemHandle&);

i32 semaphore_signal(const SemRef&);
i32 semaphore_wait(const SemRef&);
i32 semaphore_trywait(const SemRef&);
i32 semaphore_timedwait(const SemRef&, std::chrono::nanoseconds);

/// In-place storage for an unnamed semaphore.
///
/// The state itself is the synchronization object and must remain at a stable
/// address while initialized. For process-shared use it must reside in shared
/// memory.
///
/// Copy SemRef instead of copying or moving this object.
class SemHandle final {
public:
    SemHandle() noexcept = default;

    DISABLE_COPY(SemHandle);
    DISABLE_MOVE(SemHandle);

private:
    // Immutable while the semaphore is initialized.
    u32 _shared{0};

    // os_sync_wait_on_address() primitive
    alignas(sizeof(u32)) Atomic<u32> _value{0};

    friend class SemRef;

    friend i32 semaphore_init(SemHandle&, bool, u32);
    friend i32 semaphore_destroy(SemHandle&);
};

static_assert(Atomic<u32>::IS_ALWAYS_LOCK_FREE);
static_assert(sizeof(Atomic<u32>) == sizeof(u32));

/// Copyable, non-owning reference to an unnamed semaphore.
///
/// Copies and moves continue to reference the same SemHandle. This makes
/// the handle cheap and safe to pass by value between threads.
class SemRef final {
public:
    explicit SemRef(SemHandle& state) noexcept : _state(&state) {}

    SemRef(const SemRef&) noexcept = default;
    SemRef& operator=(const SemRef&) noexcept = default;

    SemRef(SemRef&&) noexcept = default;
    SemRef& operator=(SemRef&&) noexcept = default;

    [[nodiscard]] bool post() const { return darwin::semaphore_signal(*this) == 0; }

    /// Waits until the semaphore can be decremented.
    ///
    /// EINTR is retried internally. On success the caller's previous errno
    /// is restored.
    [[nodiscard]] bool wait() const {
        const auto previous = Errno::last();

        for (;;) {
            if (semaphore_wait(*this) == 0) {
                return WITH_ERRNO(previous, true);
            }

            if (!Errno::is(Errno::INTERRUPTED)) {
                return false;
            }
        }
    }

    /// Attempts to decrement the semaphore without blocking.
    ///
    /// Returns:
    ///   some(true)  - semaphore acquired
    ///   some(false) - semaphore unavailable
    ///   NONE        - error; errno contains the reason
    [[nodiscard]] Option<bool> try_wait() const {
        const auto previous = Errno::last();

        if (darwin::semaphore_trywait(*this) == 0) {
            return WITH_ERRNO(previous, true);
        }

        if (Errno::is(Errno::TRY_AGAIN)) {
            return WITH_ERRNO(previous, false);
        }

        return NONE;
    }

    /// Waits for at most `timeout` to decrement the semaphore.
    ///
    /// The timeout uses Darwin's Mach absolute-time clock and therefore is
    /// monotonic and unaffected by wall-clock changes.
    ///
    /// Returns:
    ///   some(true)  - semaphore acquired
    ///   some(false) - timeout expired
    ///   NONE        - actual error; errno contains the reason
    [[nodiscard]] Option<bool> wait_for(std::chrono::nanoseconds timeout) const {
        if (timeout <= std::chrono::nanoseconds::zero()) {
            return this->try_wait();
        }

        const auto previous = Errno::last();

        if (darwin::semaphore_timedwait(*this, timeout) == 0) {
            return WITH_ERRNO(previous, true);
        }

        if (Errno::is(Errno::TIMED_OUT)) {
            return WITH_ERRNO(previous, false);
        }

        return NONE;
    }

private:
    SemHandle* _state;

    [[nodiscard]] Atomic<u32>& value() const noexcept { return _state->_value; }

    [[nodiscard]] void* address() const noexcept { return static_cast<void*>(&_state->_value); }

    [[nodiscard]] bool shared() const noexcept { return _state->_shared != 0; }

    [[nodiscard]] os_sync_wait_on_address_flags_t wait_flags() const noexcept {
        return shared() ? OS_SYNC_WAIT_ON_ADDRESS_SHARED : OS_SYNC_WAIT_ON_ADDRESS_NONE;
    }

    [[nodiscard]] os_sync_wake_by_address_flags_t wake_flags() const noexcept {
        return shared() ? OS_SYNC_WAKE_BY_ADDRESS_SHARED : OS_SYNC_WAKE_BY_ADDRESS_NONE;
    }

    [[nodiscard]] bool try_acquire() const noexcept {
        u32 value = this->value().load(MemoryOrder::RELAXED);
        while (value != 0) {
            if (this->value().compare_exchange_weak(
                    value,
                    value - 1,
                    MemoryOrder::ACQUIRE,
                    MemoryOrder::RELAXED
                )) {
                return true;
            }
        }
        return false;
    }

    friend i32 darwin::semaphore_signal(const SemRef& handle);
    friend i32 darwin::semaphore_wait(const SemRef& handle);
    friend i32 darwin::semaphore_trywait(const SemRef& handle);
    friend i32 darwin::semaphore_timedwait(const SemRef& handle, std::chrono::nanoseconds timeout);
};

static_assert(oxide::IsTriviallyCopyable<SemRef>{});

inline i32 semaphore_init(SemHandle& state, bool shared, u32 init_value) {
    if (init_value > as<u32>(SEM_VALUE_MAX)) {
        return WITH_ERRNO(Errno::INVALID_ARGUMENT, -1);
    }

    state._shared = shared ? 1U : 0U;
    state._value.store(init_value, MemoryOrder::RELAXED);

    return 0;
}

inline i32 semaphore_destroy(SemHandle& _) {
    // os_sync_wait_on_address() does not create a persistent kernel object.
    //
    // As with POSIX sem_destroy(), the caller must ensure no thread/process
    // is still using or waiting on the semaphore.
    return 0;
}

inline i32 semaphore_signal(const SemRef& handle) {
    const auto previous = Errno::last();

    u32 value = handle.value().load(MemoryOrder::RELAXED);

    for (;;) {
        if (value == as<u32>(SEM_VALUE_MAX)) {
            return WITH_ERRNO(Errno::WOULD_OVERFLOW, -1);
        }

        if (
            handle.value()
                .compare_exchange_weak(value, value + 1, MemoryOrder::RELEASE, MemoryOrder::RELAXED)
        ) {
            break;
        }
    }

    // Wake on every post.
    //
    // Waking only when the previous count was zero can strand additional
    // sleeping waiters when multiple posts occur before the first awakened
    // waiter consumes its token.
    const i32
        result = ::os_sync_wake_by_address_any(handle.address(), sizeof(u32), handle.wake_flags());

    if (result == 0 || Errno::is(Errno::NOENT)) {
        // ENOENT means there was nobody sleeping on this address.
        return WITH_ERRNO(previous, 0);
    }

    // The token has already been published and cannot safely be rolled back:
    // another thread may already have consumed it. The documented remaining
    // wake failure is EINVAL, which means our address/size/shared-mode
    // invariants are broken.
    OXIDE_PANIC("os_sync_wake_by_address_any failed");
}

inline i32 semaphore_trywait(const SemRef& handle) {
    if (handle.try_acquire()) {
        return 0;
    }

    return WITH_ERRNO(Errno::TRY_AGAIN, -1);
}

inline i32 semaphore_wait(const SemRef& handle) {
    for (;;) {
        if (handle.try_acquire()) {
            return 0;
        }

        const i32 result = ::
            os_sync_wait_on_address(handle.address(), 0, sizeof(u32), handle.wait_flags());

        if (result >= 0) {
            // Wakeups are advisory. Compete for the token again.
            continue;
        }

        if (Errno::is(Errno::FAULT)) {
            // Apple documents EFAULT as potentially transient. Re-read the
            // atomic before attempting another wait.
            continue;
        }

        // Preserve EINTR so SemaphoreHandle::wait() can retry it using the
        // same semantics as the POSIX implementation.
        return -1;
    }
}

inline i32 semaphore_timedwait(const SemRef& handle, std::chrono::nanoseconds timeout) {
    if (handle.try_acquire()) {
        return 0;
    }

    if (timeout <= std::chrono::nanoseconds::zero()) {
        return WITH_ERRNO(Errno::TIMED_OUT, -1);
    }

    const u64 timeout_ns = as<u64>(timeout.count());

    // CLOCK_UPTIME_RAW is mach_absolute_time() expressed directly in
    // nanoseconds, which matches OS_CLOCK_MACH_ABSOLUTE_TIME semantics.
    const u64 start = ::clock_gettime_nsec_np(CLOCK_UPTIME_RAW);

    for (;;) {
        if (handle.try_acquire()) {
            return 0;
        }

        const u64 now = ::clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
        const u64 elapsed = now - start;

        if (elapsed >= timeout_ns) {
            return WITH_ERRNO(Errno::TIMED_OUT, -1);
        }

        const i32 result = ::os_sync_wait_on_address_with_timeout(
            handle.address(),
            0,
            sizeof(u32),
            handle.wait_flags(),
            OS_CLOCK_MACH_ABSOLUTE_TIME,
            timeout_ns - elapsed
        );

        if (result >= 0) {
            continue;
        }

        if (Errno::is(Errno::TIMED_OUT)) {
            return -1;
        }

        if (Errno::is(Errno::INTERRUPTED) || Errno::is(Errno::FAULT)) {
            // Recheck the value and recompute the remaining timeout. This
            // prevents signal handling or spurious wakeups from extending
            // the requested duration.
            continue;
        }

        return -1;
    }
}

} // namespace oxide::posix::darwin

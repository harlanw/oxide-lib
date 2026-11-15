#include <oxide/posix/common/semaphore.hpp>

#include <sched.h>

#include <ctime>

#include <oxide/kernel/processor.hpp>
#include <oxide/posix/pal/errno.hpp>

#if defined(__linux__) && defined(__GLIBC__) && defined(__GLIBC_PREREQ) // clang-format off
    #if __GLIBC_PREREQ(2, 30)
        #define OXIDE_HAS_SEM_CLOCKWAIT 1
    #else
        #define OXIDE_HAS_SEM_CLOCKWAIT 0
    #endif
#else
    #define OXIDE_HAS_SEM_CLOCKWAIT 0
#endif // clang-format on

namespace oxide::posix {

using namespace std::chrono_literals;

using Seconds = std::chrono::seconds;
using Nanoseconds = std::chrono::nanoseconds;

inline constexpr u64 SEMAPHORE_SPIN_REPETITIONS = 64;
inline constexpr u64 SEMAPHORE_YIELD_REPETITIONS = 32;

// Used only by the portable timed-wait fallback. Platforms with sem_clockwait()
// block in the kernel instead.
inline constexpr Nanoseconds SEMAPHORE_POLL_INTERVAL = 250us;

namespace {

[[nodiscard]] Option<timespec> make_deadline(Nanoseconds timeout) {
    using namespace std::chrono;

    timespec deadline{};
    if (::clock_gettime(CLOCK_MONOTONIC, &deadline) != 0) {
        return NONE;
    }

    const auto secs = duration_cast<Seconds>(timeout);
    const auto nanos = duration_cast<Nanoseconds>(timeout - secs);

    const bool carry = deadline.tv_nsec + nanos.count() >= 1'000'000'000L;
    const auto added_secs = secs.count() + (carry ? 1 : 0);

    if (added_secs > std::numeric_limits<time_t>::max() - deadline.tv_sec) {
        return WITH_ERRNO(Errno::WOULD_OVERFLOW, NONE);
    }

    deadline.tv_sec += as<time_t>(added_secs);
    deadline.tv_nsec += as<long>(nanos.count());

    if (deadline.tv_nsec >= 1'000'000'000L) {
        deadline.tv_nsec -= 1'000'000'000L;
    }

    return deadline;
}

#if !OXIDE_HAS_SEM_CLOCKWAIT
[[nodiscard]] Option<Nanoseconds> remaining_until(const timespec& deadline) {
    timespec now{};
    if (::clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return NONE;
    }

    if (now.tv_sec > deadline.tv_sec
        || (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec)) {
        return some(Nanoseconds::zero());
    }

    i64 secs = as<i64>(deadline.tv_sec) - as<i64>(now.tv_sec);
    i64 nanos = as<i64>(deadline.tv_nsec) - as<i64>(now.tv_nsec);

    if (nanos < 0) {
        --secs;
        nanos += 1'000'000'000;
    }

    return Seconds{secs} + Nanoseconds{nanos};
}

[[nodiscard]] bool sleep_for(Nanoseconds duration) {
    using namespace std::chrono;

    if (duration <= Nanoseconds::zero()) {
        return true;
    }

    const auto secs = duration_cast<Seconds>(duration);
    const auto nanos = duration_cast<Nanoseconds>(duration - secs);

    const timespec requested{
        as<time_t>(secs.count()),
        as<long>(nanos.count()),
    };

    // This is one backoff step, not a promise to sleep for the whole duration.
    // After EINTR, let the caller recheck its absolute deadline so time spent
    // handling signals counts toward the timeout.
    const auto previous = Errno::last();

    if (::nanosleep(&requested, nullptr) != 0 && Errno::is_not(Errno::INTERRUPTED)) {
        return false;
    }

    return WITH_ERRNO(previous, true);
}

[[nodiscard]] bool backoff(u64 attempt, Nanoseconds remaining) {
    if (attempt < SEMAPHORE_SPIN_REPETITIONS) {
        kernel::cpu_relax();
        return true;
    }

    if (attempt < SEMAPHORE_SPIN_REPETITIONS + SEMAPHORE_YIELD_REPETITIONS) {
        hints::discard(::sched_yield());
        return true;
    }

    return sleep_for(std::min(remaining, SEMAPHORE_POLL_INTERVAL));
}
#endif

} // namespace

[[nodiscard]] bool sem_wait(sem_t* handle) {
    const auto previous = Errno::last();

    for (;;) {
        if (::sem_wait(handle) == 0) {
            return WITH_ERRNO(previous, true);
        }

        if (!Errno::last().interrupted()) {
            return false;
        }
    }
}

[[nodiscard]] Option<bool> sem_trywait(sem_t* handle) {
    const auto previous = Errno::last();

    if (::sem_trywait(handle) == 0) {
        return WITH_ERRNO(previous, true);
    }

    if (Errno::is(Errno::TRY_AGAIN)) {
        return WITH_ERRNO(previous, false);
    }

    return NONE;
}

[[nodiscard]] Option<bool> sem_waitfor(sem_t* handle, Nanoseconds timeout) {
    if (hints::unlikely(timeout <= Nanoseconds::zero())) {
        return posix::sem_trywait(handle);
    }

    const auto previous = Errno::last();

    auto deadline = make_deadline(timeout);
    if (deadline.is_none()) {
        return NONE;
    }

    // clang-format off
    #if OXIDE_HAS_SEM_CLOCKWAIT
        for (;;) {
            if (::sem_clockwait(handle, CLOCK_MONOTONIC, &*deadline) == 0) {
                return WITH_ERRNO(previous, true);
            }

            const auto last = Errno::last();

            if (last.timed_out()) {
                return WITH_ERRNO(previous, false);
            }

            if (!last.interrupted()) {
                return NONE;
            }
        }
    #else
        for (u64 attempt = 0;; ++attempt) {
            auto result = posix::sem_trywait(handle);
            if (result.is_none()) {
                return NONE;
            }
            if (*result) {
                return WITH_ERRNO(previous, true);
            }

            auto remaining = remaining_until(*deadline);
            if (remaining.is_none()) {
                return NONE;
            }

            if (*remaining <= Nanoseconds::zero()) {
                return WITH_ERRNO(previous, false);
            }

            if (!backoff(attempt, *remaining)) {
                return NONE;
            }
        }
    #endif
    // clang-format on
}

} // namespace oxide::posix

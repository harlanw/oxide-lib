#pragma once

#include <unistd.h>

#include <algorithm>
#include <climits>

#include <oxide/compiler/target.hpp>
#include <oxide/types.hpp>

#define SYSCONF(type, name, key) \
    [[nodiscard]] static type name() noexcept { \
        return query<type>(key); \
    }

namespace oxide::posix {

#if OXIDE_TARGET_OS_MACOS // clang-format off
    inline constexpr bool HAS_PERSISTENT_SHARED_MEMORY{false};
    inline constexpr bool HAS_NAMED_SEMAPHORES{false};
#elif OXIDE_TARGET_OS_LINUX
    inline constexpr bool HAS_PERSISTENT_SHARED_MEMORY{true};
    inline constexpr bool HAS_NAMED_SEMAPHORES{true};
#endif // clang-format on

template <typename T>
[[nodiscard]] inline T query(i32 key) noexcept {
    const long value = ::sysconf(key);
    return as<T>(std::clamp(value, 0L, LONG_MAX));
}

struct SystemFeature final {
    SYSCONF(bool, barriers, _SC_BARRIERS);
    SYSCONF(bool, asynchronous_io, _SC_ASYNCHRONOUS_IO);
    SYSCONF(bool, clock_selection, _SC_CLOCK_SELECTION);
    SYSCONF(bool, job_control, _SC_JOB_CONTROL);
    SYSCONF(bool, mapped_files, _SC_MAPPED_FILES);
    SYSCONF(bool, memory_protection, _SC_MEMORY_PROTECTION);
    SYSCONF(bool, monotonic_clock, _SC_MONOTONIC_CLOCK);
    SYSCONF(bool, raw_sockets, _SC_RAW_SOCKETS);
    SYSCONF(bool, reader_writer_locks, _SC_READER_WRITER_LOCKS);
    SYSCONF(bool, realtime_signals, _SC_REALTIME_SIGNALS);
    SYSCONF(bool, saved_user_and_group_ids, _SC_SAVED_IDS);
    SYSCONF(bool, semaphores, _SC_SEMAPHORES);
    SYSCONF(bool, shared_memory_objects, _SC_SHARED_MEMORY_OBJECTS);
    SYSCONF(bool, shell, _SC_SHELL);
    SYSCONF(bool, spin_locks, _SC_SPIN_LOCKS);
    SYSCONF(bool, thread_safe_functions, _SC_THREAD_SAFE_FUNCTIONS);
    SYSCONF(bool, threads, _SC_THREADS);
    SYSCONF(bool, timeouts, _SC_TIMEOUTS);
    SYSCONF(bool, timers, _SC_TIMERS);
    SYSCONF(bool, xopen_realtime, _SC_XOPEN_REALTIME);
    SYSCONF(bool, xopen_realtime_threads, _SC_XOPEN_REALTIME_THREADS);

    [[nodiscard]] static constexpr bool has_sem_clockwait() noexcept { // clang-format off
        #if defined(__linux__) && defined(__GLIBC__) && defined(__GLIBC_PREREQ)
            return static_cast<bool>(__GLIBC_PREREQ(2, 30));
        #else
            return false;
        #endif // clang-format on
    }
};

struct SystemInfo final {
    SYSCONF(i32, posix_version, _SC_VERSION);
    SYSCONF(usize, page_size, _SC_PAGESIZE);
    SYSCONF(i64, clock_ticks_per_second, _SC_CLK_TCK);
    SYSCONF(usize, configured_processor_count, _SC_NPROCESSORS_CONF);
    SYSCONF(usize, online_processor_count, _SC_NPROCESSORS_ONLN);
};

struct SystemLimit final {
    SYSCONF(usize, max_hostname_length, _SC_HOST_NAME_MAX);
    SYSCONF(usize, max_login_name_length, _SC_LOGIN_NAME_MAX);
    SYSCONF(usize, max_supplementary_group_count, _SC_NGROUPS_MAX);
    SYSCONF(usize, max_open_file_count, _SC_OPEN_MAX);
    SYSCONF(usize, max_process_count_per_user, _SC_CHILD_MAX);
    SYSCONF(usize, max_argument_length, _SC_ARG_MAX);
    SYSCONF(usize, max_open_stream_count, _SC_STREAM_MAX);
    SYSCONF(usize, max_symbolic_link_count, _SC_SYMLOOP_MAX);
    SYSCONF(usize, max_terminal_device_name_length, _SC_TTY_NAME_MAX);
    SYSCONF(usize, max_timezone_name_length, _SC_TZNAME_MAX);
    SYSCONF(usize, max_semaphore_count, _SC_SEM_NSEMS_MAX);
    SYSCONF(usize, max_semaphore_value, _SC_SEM_VALUE_MAX);
    SYSCONF(usize, max_open_message_queue_count, _SC_MQ_OPEN_MAX);
    SYSCONF(usize, max_message_queue_priority, _SC_MQ_PRIO_MAX);
    SYSCONF(usize, max_thread_count, _SC_THREAD_THREADS_MAX);
    SYSCONF(usize, max_password_buffer_size, _SC_GETPW_R_SIZE_MAX);
    SYSCONF(usize, max_group_buffer_size, _SC_GETGR_R_SIZE_MAX);
    SYSCONF(usize, min_thread_stack_size, _SC_THREAD_STACK_MIN);
};

struct SystemOption final {
    SYSCONF(bool, advisory_info, _SC_ADVISORY_INFO);
    SYSCONF(bool, cpu_time, _SC_CPUTIME);
    SYSCONF(bool, fsync, _SC_FSYNC);
    SYSCONF(bool, ipv6, _SC_IPV6);
    SYSCONF(bool, memory_locking, _SC_MEMLOCK);
    SYSCONF(bool, memory_range_locking, _SC_MEMLOCK_RANGE);
    SYSCONF(bool, message_passing, _SC_MESSAGE_PASSING);
    SYSCONF(bool, prioritized_io, _SC_PRIORITIZED_IO);
    SYSCONF(bool, priority_scheduling, _SC_PRIORITY_SCHEDULING);
    SYSCONF(bool, regular_expressions, _SC_REGEXP);
    SYSCONF(bool, spawn, _SC_SPAWN);
    SYSCONF(bool, process_sporadic_server, _SC_SPORADIC_SERVER);
    SYSCONF(bool, synchronized_io, _SC_SYNCHRONIZED_IO);
    SYSCONF(bool, thread_stack_address, _SC_THREAD_ATTR_STACKADDR);
    SYSCONF(bool, thread_stack_size, _SC_THREAD_ATTR_STACKSIZE);
    SYSCONF(bool, thread_cpu_time, _SC_THREAD_CPUTIME);
    SYSCONF(bool, thread_priority_scheduling, _SC_THREAD_PRIORITY_SCHEDULING);
    SYSCONF(bool, process_shared_synchronization, _SC_THREAD_PROCESS_SHARED);
    SYSCONF(bool, mutex_priority_inheritance, _SC_THREAD_PRIO_INHERIT);
    SYSCONF(bool, mutex_priority_protection, _SC_THREAD_PRIO_PROTECT);
    SYSCONF(bool, thread_sporadic_server, _SC_THREAD_SPORADIC_SERVER);
    SYSCONF(bool, trace, _SC_TRACE);
    SYSCONF(bool, trace_event_filter, _SC_TRACE_EVENT_FILTER);
    SYSCONF(bool, trace_inherit, _SC_TRACE_INHERIT);
    SYSCONF(bool, trace_log, _SC_TRACE_LOG);
    SYSCONF(bool, typed_memory_objects, _SC_TYPED_MEMORY_OBJECTS);
};

} // namespace oxide::posix

#undef SYSCONF

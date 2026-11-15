#include <cerrno>
#include <mutex>
#include <new>

// clang-format off
#if OXIDE_TARGET_OS_WINDOWS
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif !OXIDE_TARGET_OS_NONE
    #include <csignal>
    #include <pthread.h>
    #include <unistd.h>
#endif
// clang-format on

#include <oxide/compiler/target.hpp>
#include <oxide/io.hpp>
#include <oxide/mem/scope.hpp>
#include <oxide/panic.hpp>

namespace oxide::io {

namespace {

[[noreturn]] void print_failed(Error error) {
    OXIDE_PANIC("failed printing to stdout: {}", error.to_string());
}

#if !OXIDE_TARGET_OS_NONE // clang-format off
    std::recursive_mutex& stdout_mutex() {
        // Keep the lock alive for printing from static destructors, without a
        // heap allocation or a dependency on the lifetime of C's stdout FILE.
        alignas(std::recursive_mutex) static unsigned char storage[sizeof(std::recursive_mutex)];
        static auto* mutex = ::new (static_cast<void*>(storage)) std::recursive_mutex;
        return *mutex;
    }
#endif // clang-format on

} // namespace

void detail::PrintBuffer::flush() {
    if (_len == 0) {
        return;
    }

    const usize len = std::exchange(_len, 0);

    // clang-format off
    #if OXIDE_TARGET_OS_WINDOWS
        const HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
            return;
        }
        usize written = 0;
        while (written < len) {
            DWORD count = 0;
            if (!::WriteFile(handle, _data + written, static_cast<DWORD>(len - written), &count, nullptr)) {
                const DWORD code = ::GetLastError();
                if (code == ERROR_INVALID_HANDLE) {
                    return;
                }
                OXIDE_PANIC("failed printing to stdout: Windows error {}", code);
            }
            if (count == 0) {
                print_failed(Error{ErrorKind::WriteZero});
            }
            written += count;
        }
    #elif OXIDE_TARGET_OS_NONE
        print_failed(Error{ErrorKind::Unsupported});
    #else
        // Rust's runtime ignores SIGPIPE. Block it for these writes without
        // changing the process-wide disposition, so EPIPE reaches the panic.
        sigset_t blocked;
        sigset_t previous;
        ::sigemptyset(&blocked);
        ::sigaddset(&blocked, SIGPIPE);
        const int block_error = ::pthread_sigmask(SIG_BLOCK, &blocked, &previous);
        if (block_error != 0) {
            print_failed(Error::from_raw_os_error(block_error));
        }
        auto restore = mem::defer([&]() noexcept {
            const int code = ::pthread_sigmask(SIG_SETMASK, &previous, nullptr);
            if (code != 0) {
                print_failed(Error::from_raw_os_error(code));
            }
        });

        usize written = 0;
        while (written < len) {
            const ssize_t count = ::write(STDOUT_FILENO, _data + written, len - written);
            if (count > 0) {
                written += static_cast<usize>(count);
            } else if (count == 0) {
                print_failed(Error{ErrorKind::WriteZero});
            } else {
                const int code = errno;
                if (code == EINTR) {
                    continue;
                }
                if (code == EBADF) {
                    return;
                }
                print_failed(Error::from_raw_os_error(code));
            }
        }
    #endif
    // clang-format on
}

void _print(detail::PrintFn format, const void* args) {
    // Keep the reentrant lock across formatting and every output chunk.
    // clang-format off
    #if !OXIDE_TARGET_OS_NONE
        const std::lock_guard<std::recursive_mutex> lock(stdout_mutex());
    #endif // clang-format on

    detail::PrintBuffer buffer;
    format(buffer, args);
    buffer.flush();
}

bool is_terminal() noexcept { // clang-format off
    #if OXIDE_TARGET_OS_WINDOWS
        DWORD mode{};
        return ::GetConsoleMode(::GetStdHandle(STD_OUTPUT_HANDLE), &mode) != 0;
    #elif OXIDE_TARGET_OS_NONE
        return false;
    #else
        return ::isatty(STDOUT_FILENO) == 1;
    #endif
} // clang-format on

} // namespace oxide::io

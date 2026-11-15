#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>

#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string.test.hpp"

namespace {

bool fail_allocation = false;
std::size_t allocation_attempts = 0;
std::size_t live_allocations = 0;
std::size_t last_allocation_size = 0;

void* allocate_array(std::size_t size) noexcept {
    ++allocation_attempts;
    last_allocation_size = size;
    if (fail_allocation) {
        return nullptr;
    }
    void* pointer = std::malloc(size == 0 ? 1 : size);
    if (pointer != nullptr) {
        ++live_allocations;
    }
    return pointer;
}

void expect_panic(void (*operation)(), const char* message) {
    int pipe_fds[2];
    OXIDE_ASSERT(pipe(pipe_fds) == 0);
    const pid_t child = fork();
    OXIDE_ASSERT(child >= 0);
    if (child == 0) {
        const rlimit limit{0, 0};
        if (setrlimit(RLIMIT_CORE, &limit) != 0 || dup2(pipe_fds[1], STDERR_FILENO) < 0) {
            std::_Exit(2);
        }
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        operation();
        std::_Exit(0);
    }

    close(pipe_fds[1]);
    std::string diagnostic;
    char buffer[256];
    for (;;) {
        const auto count = read(pipe_fds[0], buffer, sizeof(buffer));
        if (count < 0 && errno == EINTR) {
            continue;
        }
        OXIDE_ASSERT(count >= 0);
        if (count == 0) {
            break;
        }
        diagnostic.append(buffer, static_cast<std::size_t>(count));
    }
    close(pipe_fds[0]);
    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    OXIDE_ASSERT(waited == child);
    OXIDE_ASSERT(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    OXIDE_ASSERT(diagnostic.find(message) != std::string::npos);
    OXIDE_ASSERT(diagnostic.find("runtime error:") == std::string::npos);
    OXIDE_ASSERT(diagnostic.find("ERROR: AddressSanitizer") == std::string::npos);
}

// Use distinct parameters to exercise self-move without a compiler self-move warning.
[[gnu::noinline]] void move_assign(oxide::String& target, oxide::String& source) {
    target = std::move(source);
}

void check_allocations() {
    using oxide::String;
    auto before = allocation_attempts;
    {
        String empty;
        auto clone = empty.clone();
        auto reserved = String::with_capacity(0);
        empty.reserve(0);
        empty.reserve_exact(0);
        empty.push_str(std::string_view{});
        OXIDE_ASSERT(allocation_attempts == before);
    }
    OXIDE_ASSERT(live_allocations == 0);

    {
        auto value = String::with_capacity(16);
        OXIDE_ASSERT(last_allocation_size == value.capacity());
        before = allocation_attempts;
        value.push_str("abc");
        value.push_str(value.as_str());
        value.push('!');
        value.truncate(3);
        value.clear();
        value.push_str("reuse");
        OXIDE_ASSERT(allocation_attempts == before);
        auto pointer = value.data();
        String moved(std::move(value));
        OXIDE_ASSERT(allocation_attempts == before && moved.data() == pointer);
        String target("old");
        before = allocation_attempts;
        target = std::move(moved);
        move_assign(target, target);
        OXIDE_ASSERT(allocation_attempts == before && target.data() == pointer);
        oxide::tests::string::check(target, "reuse");
    }
    OXIDE_ASSERT(live_allocations == 0);

    {
        auto value = String::with_capacity(1);
        OXIDE_ASSERT(last_allocation_size == 1);
        before = allocation_attempts;
        value.push('x');
        OXIDE_ASSERT(value.len() == value.capacity() && allocation_attempts == before);
        value.shrink_to_fit();
        OXIDE_ASSERT(allocation_attempts == before);
        value.clear();
        value.shrink_to_fit();
        OXIDE_ASSERT(value.capacity() == 0 && live_allocations == 0);
    }
}

} // namespace

[[gnu::noinline]] void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return allocate_array(size);
}

[[gnu::noinline]] void* operator new[](std::size_t size) {
    if (void* pointer = allocate_array(size)) {
        return pointer;
    }
    std::abort();
}

[[gnu::noinline]] void operator delete[](void* pointer) noexcept {
    if (pointer != nullptr) {
        --live_allocations;
        std::free(pointer);
    }
}

void operator delete[](void* pointer, std::size_t) noexcept {
    ::operator delete[](pointer);
}

void operator delete[](void* pointer, const std::nothrow_t&) noexcept {
    ::operator delete[](pointer);
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t*, std::size_t);

int main() {
    oxide::tests::string::run();
    OXIDE_ASSERT(live_allocations == 0);
    check_allocations();

    expect_panic(
        [] {
            fail_allocation = true;
            auto value = oxide::String::with_capacity(16);
            std::fprintf(
                stderr,
                "unexpected allocation success: %p",
                static_cast<void*>(value.data())
            );
        },
        "String allocation failed"
    );
    expect_panic(
        [] {
            oxide::String value("abc");
            value.reserve(std::numeric_limits<oxide::usize>::max());
        },
        "String capacity overflow"
    );
    expect_panic(
        [] {
            oxide::String value;
            value.reserve_exact(std::numeric_limits<oxide::usize>::max());
        },
        "String capacity overflow"
    );
    expect_panic(
        [] {
            oxide::String value("abc");
            std::fprintf(stderr, "%c", value[value.len()]);
        },
        "String index out of bounds"
    );

    LLVMFuzzerTestOneInput(nullptr, 0);
    std::uint8_t input[256];
    std::uint32_t random = 0x12345678;
    for (std::size_t seed = 0; seed < 5000; ++seed) {
        for (auto& byte : input) {
            random = random * 1664525u + 1013904223u;
            byte = static_cast<std::uint8_t>(random >> 24);
        }
        LLVMFuzzerTestOneInput(input, seed % (sizeof(input) + 1));
        OXIDE_ASSERT(live_allocations == 0);
    }
    std::puts("String regression, allocation, panic, and 5,000 fuzz-input checks passed.");
}

#pragma once

#include <oxide/compiler/target.hpp>

// clang-format off
#if !(OXIDE_TARGET_ARCH_X86_64 || OXIDE_TARGET_ARCH_X86 || \
      OXIDE_TARGET_ARCH_AARCH64 || OXIDE_TARGET_ARCH_ARM)
    #include <thread>
#endif

#if OXIDE_TARGET_ENV_MSVC
    #include <intrin.h>
#endif
// clang-format on

namespace oxide::kernel {

// Hint to the CPU that this thread is busy-waiting.
inline void cpu_relax() noexcept {
    // clang-format off
    #if OXIDE_TARGET_ARCH_X86_64 || OXIDE_TARGET_ARCH_X86
        #if OXIDE_TARGET_ENV_MSVC
            _mm_pause();
        #else
            __builtin_ia32_pause();
        #endif
    #elif OXIDE_TARGET_ARCH_AARCH64 || OXIDE_TARGET_ARCH_ARM
        #if OXIDE_TARGET_ENV_MSVC
            __yield();
        #else
            asm volatile("yield");
        #endif
    #else
        std::this_thread::yield();
    #endif
    // clang-format on
}

} // namespace oxide::kernel

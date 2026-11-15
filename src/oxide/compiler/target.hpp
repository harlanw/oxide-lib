#pragma once

// clang-format off
#if defined(_M_ARM64EC)
    #define OXIDE_TARGET_ARCH_AARCH64 1
#elif defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
    #define OXIDE_TARGET_ARCH_X86_64 1
#elif defined(__i386__) || defined(_M_IX86)
    #define OXIDE_TARGET_ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define OXIDE_TARGET_ARCH_AARCH64 1
#elif defined(__arm__) || defined(_M_ARM)
    #define OXIDE_TARGET_ARCH_ARM 1
#elif defined(__riscv) && defined(__riscv_xlen)
    #if __riscv_xlen == 64
        #define OXIDE_TARGET_ARCH_RISCV64 1
    #elif __riscv_xlen == 32
        #define OXIDE_TARGET_ARCH_RISCV32 1
    #else
        #error "Unsupported RISC-V architecture"
    #endif
#else
    #error "Unsupported target architecture"
#endif

#ifndef OXIDE_TARGET_ARCH_X86_64
    #define OXIDE_TARGET_ARCH_X86_64 0
#endif
#ifndef OXIDE_TARGET_ARCH_X86
    #define OXIDE_TARGET_ARCH_X86 0
#endif
#ifndef OXIDE_TARGET_ARCH_AARCH64
    #define OXIDE_TARGET_ARCH_AARCH64 0
#endif
#ifndef OXIDE_TARGET_ARCH_ARM
    #define OXIDE_TARGET_ARCH_ARM 0
#endif
#ifndef OXIDE_TARGET_ARCH_RISCV64
    #define OXIDE_TARGET_ARCH_RISCV64 0
#endif
#ifndef OXIDE_TARGET_ARCH_RISCV32
    #define OXIDE_TARGET_ARCH_RISCV32 0
#endif

#if defined(OXIDE_TARGET_OS_NONE) && OXIDE_TARGET_OS_NONE
    #if OXIDE_TARGET_OS_NONE != 1
        #error "OXIDE_TARGET_OS_NONE must be 0 or 1"
    #endif
#else
    #if defined(__is_target_os)
        #define OXIDE_DETAIL_IS_TARGET_OS(name) __is_target_os(name)
    #else
        #define OXIDE_DETAIL_IS_TARGET_OS(name) 0
    #endif

    #if defined(__ANDROID__)
        #define OXIDE_TARGET_OS_ANDROID 1
    #elif defined(__linux__)
        #define OXIDE_TARGET_OS_LINUX 1
    #elif defined(_WIN32)
        #define OXIDE_TARGET_OS_WINDOWS 1
    #elif defined(__APPLE__) && defined(__MACH__)
        #if OXIDE_DETAIL_IS_TARGET_OS(xros) || (defined(TARGET_OS_VISION) && TARGET_OS_VISION)
            #define OXIDE_TARGET_OS_VISIONOS 1
        #elif defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__)
            #define OXIDE_TARGET_OS_TVOS 1
        #elif defined(__ENVIRONMENT_WATCH_OS_VERSION_MIN_REQUIRED__)
            #define OXIDE_TARGET_OS_WATCHOS 1
        #elif defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
            #define OXIDE_TARGET_OS_IOS 1
        #elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
            #define OXIDE_TARGET_OS_MACOS 1
        #else
            #error "Unsupported Apple target operating system"
        #endif
    #elif defined(__FreeBSD__)
        #define OXIDE_TARGET_OS_FREEBSD 1
    #elif defined(__DragonFly__)
        #define OXIDE_TARGET_OS_DRAGONFLY 1
    #elif defined(__OpenBSD__)
        #define OXIDE_TARGET_OS_OPENBSD 1
    #elif defined(__NetBSD__)
        #define OXIDE_TARGET_OS_NETBSD 1
    #elif defined(__Fuchsia__)
        #define OXIDE_TARGET_OS_FUCHSIA 1
    #else
        #error "Unsupported target Os; define OXIDE_TARGET_OS_NONE=1 for a bare-metal target"
    #endif

    #undef OXIDE_DETAIL_IS_TARGET_OS
#endif

#ifndef OXIDE_TARGET_OS_WINDOWS
    #define OXIDE_TARGET_OS_WINDOWS 0
#endif
#ifndef OXIDE_TARGET_OS_MACOS
    #define OXIDE_TARGET_OS_MACOS 0
#endif
#ifndef OXIDE_TARGET_OS_IOS
    #define OXIDE_TARGET_OS_IOS 0
#endif
#ifndef OXIDE_TARGET_OS_TVOS
    #define OXIDE_TARGET_OS_TVOS 0
#endif
#ifndef OXIDE_TARGET_OS_WATCHOS
    #define OXIDE_TARGET_OS_WATCHOS 0
#endif
#ifndef OXIDE_TARGET_OS_VISIONOS
    #define OXIDE_TARGET_OS_VISIONOS 0
#endif
#ifndef OXIDE_TARGET_OS_LINUX
    #define OXIDE_TARGET_OS_LINUX 0
#endif
#ifndef OXIDE_TARGET_OS_ANDROID
    #define OXIDE_TARGET_OS_ANDROID 0
#endif
#ifndef OXIDE_TARGET_OS_FREEBSD
    #define OXIDE_TARGET_OS_FREEBSD 0
#endif
#ifndef OXIDE_TARGET_OS_DRAGONFLY
    #define OXIDE_TARGET_OS_DRAGONFLY 0
#endif
#ifndef OXIDE_TARGET_OS_OPENBSD
    #define OXIDE_TARGET_OS_OPENBSD 0
#endif
#ifndef OXIDE_TARGET_OS_NETBSD
    #define OXIDE_TARGET_OS_NETBSD 0
#endif
#ifndef OXIDE_TARGET_OS_FUCHSIA
    #define OXIDE_TARGET_OS_FUCHSIA 0
#endif
#ifndef OXIDE_TARGET_OS_NONE
    #define OXIDE_TARGET_OS_NONE 0
#endif

#if (OXIDE_TARGET_OS_WINDOWS + OXIDE_TARGET_OS_MACOS + OXIDE_TARGET_OS_IOS + \
     OXIDE_TARGET_OS_TVOS + OXIDE_TARGET_OS_WATCHOS + OXIDE_TARGET_OS_VISIONOS + \
     OXIDE_TARGET_OS_LINUX + OXIDE_TARGET_OS_ANDROID + OXIDE_TARGET_OS_FREEBSD + \
     OXIDE_TARGET_OS_DRAGONFLY + OXIDE_TARGET_OS_OPENBSD + OXIDE_TARGET_OS_NETBSD + \
     OXIDE_TARGET_OS_FUCHSIA + OXIDE_TARGET_OS_NONE) != 1
    #error "Exactly one target operating system must be selected"
#endif

#if defined(__is_target_environment)
    #define OXIDE_DETAIL_IS_TARGET_ENV(name) __is_target_environment(name)
#else
    #define OXIDE_DETAIL_IS_TARGET_ENV(name) 0
#endif

// Define one OXIDE_TARGET_ENV_* flag to 1 to override detection.
#if !defined(OXIDE_TARGET_ENV_GNU) && !defined(OXIDE_TARGET_ENV_MSVC) && \
    !defined(OXIDE_TARGET_ENV_MUSL) && !defined(OXIDE_TARGET_ENV_NONE)
    #if OXIDE_DETAIL_IS_TARGET_ENV(msvc) || defined(_MSC_VER)
        #define OXIDE_TARGET_ENV_MSVC 1
    #elif OXIDE_DETAIL_IS_TARGET_ENV(musl) || OXIDE_DETAIL_IS_TARGET_ENV(musleabi) || \
          OXIDE_DETAIL_IS_TARGET_ENV(musleabihf) || OXIDE_DETAIL_IS_TARGET_ENV(muslx32)
        #define OXIDE_TARGET_ENV_MUSL 1
    #elif OXIDE_DETAIL_IS_TARGET_ENV(gnu) || OXIDE_DETAIL_IS_TARGET_ENV(gnueabi) || \
          OXIDE_DETAIL_IS_TARGET_ENV(gnueabihf) || OXIDE_DETAIL_IS_TARGET_ENV(gnux32) || \
          OXIDE_DETAIL_IS_TARGET_ENV(gnu_ilp32) || defined(__MINGW32__)
        #define OXIDE_TARGET_ENV_GNU 1
    #elif OXIDE_TARGET_OS_LINUX
        #if defined(__has_include)
            #if __has_include(<features.h>)
                #include <features.h>
            #endif
        #endif
        #if defined(__GLIBC__)
            #define OXIDE_TARGET_ENV_GNU 1
        #else
            #error "Cannot determine target environment; define OXIDE_TARGET_ENV_GNU=1 or OXIDE_TARGET_ENV_MUSL=1"
        #endif
    #else
        #define OXIDE_TARGET_ENV_NONE 1
    #endif
#endif

#ifndef OXIDE_TARGET_ENV_GNU
    #define OXIDE_TARGET_ENV_GNU 0
#endif
#ifndef OXIDE_TARGET_ENV_MSVC
    #define OXIDE_TARGET_ENV_MSVC 0
#endif
#ifndef OXIDE_TARGET_ENV_MUSL
    #define OXIDE_TARGET_ENV_MUSL 0
#endif
#ifndef OXIDE_TARGET_ENV_NONE
    #define OXIDE_TARGET_ENV_NONE 0
#endif

#if (OXIDE_TARGET_ENV_GNU != 0 && OXIDE_TARGET_ENV_GNU != 1) || \
    (OXIDE_TARGET_ENV_MSVC != 0 && OXIDE_TARGET_ENV_MSVC != 1) || \
    (OXIDE_TARGET_ENV_MUSL != 0 && OXIDE_TARGET_ENV_MUSL != 1) || \
    (OXIDE_TARGET_ENV_NONE != 0 && OXIDE_TARGET_ENV_NONE != 1) || \
    (OXIDE_TARGET_ENV_GNU + OXIDE_TARGET_ENV_MSVC + OXIDE_TARGET_ENV_MUSL + OXIDE_TARGET_ENV_NONE) != 1
    #error "Exactly one target environment flag must be 1; all others must be 0"
#endif

#if defined(__SIZEOF_POINTER__) && defined(__CHAR_BIT__)
    #define OXIDE_TARGET_POINTER_WIDTH (__SIZEOF_POINTER__ * __CHAR_BIT__)
#elif defined(_WIN64)
    #define OXIDE_TARGET_POINTER_WIDTH 64
#elif defined(_WIN32)
    #define OXIDE_TARGET_POINTER_WIDTH 32
#else
    #error "Unsupported compiler: cannot determine target pointer width"
#endif

// Define one OXIDE_TARGET_ABI_* flag to 1 to override detection. LLVM, UWP,
// and SOFTFLOAT require explicit selection when compiler macros cannot identify them.
#if !defined(OXIDE_TARGET_ABI_EABI) && !defined(OXIDE_TARGET_ABI_EABIHF) && \
    !defined(OXIDE_TARGET_ABI_X32) && !defined(OXIDE_TARGET_ABI_ILP32) && \
    !defined(OXIDE_TARGET_ABI_MACABI) && !defined(OXIDE_TARGET_ABI_SIM) && \
    !defined(OXIDE_TARGET_ABI_SOFTFLOAT) && !defined(OXIDE_TARGET_ABI_LLVM) && \
    !defined(OXIDE_TARGET_ABI_UWP) && !defined(OXIDE_TARGET_ABI_NONE)
    #if OXIDE_DETAIL_IS_TARGET_ENV(macabi) || \
        (defined(TARGET_OS_MACCATALYST) && TARGET_OS_MACCATALYST)
        #define OXIDE_TARGET_ABI_MACABI 1
    #elif OXIDE_DETAIL_IS_TARGET_ENV(simulator) || \
          (defined(TARGET_OS_SIMULATOR) && TARGET_OS_SIMULATOR) || \
          ((OXIDE_TARGET_ARCH_X86 || OXIDE_TARGET_ARCH_X86_64) && \
           (OXIDE_TARGET_OS_IOS || OXIDE_TARGET_OS_TVOS || OXIDE_TARGET_OS_WATCHOS))
        #define OXIDE_TARGET_ABI_SIM 1
    #elif OXIDE_TARGET_ARCH_ARM && defined(__ARM_EABI__)
        #if defined(__ARM_PCS_VFP)
            #define OXIDE_TARGET_ABI_EABIHF 1
        #else
            #define OXIDE_TARGET_ABI_EABI 1
        #endif
    #elif OXIDE_TARGET_ARCH_X86_64 && OXIDE_TARGET_POINTER_WIDTH == 32
        #define OXIDE_TARGET_ABI_X32 1
    #elif OXIDE_TARGET_ARCH_AARCH64 && OXIDE_TARGET_POINTER_WIDTH == 32 && !defined(__APPLE__)
        #define OXIDE_TARGET_ABI_ILP32 1
    #else
        #define OXIDE_TARGET_ABI_NONE 1
    #endif
#endif

#undef OXIDE_DETAIL_IS_TARGET_ENV

#ifndef OXIDE_TARGET_ABI_EABI
    #define OXIDE_TARGET_ABI_EABI 0
#endif
#ifndef OXIDE_TARGET_ABI_EABIHF
    #define OXIDE_TARGET_ABI_EABIHF 0
#endif
#ifndef OXIDE_TARGET_ABI_X32
    #define OXIDE_TARGET_ABI_X32 0
#endif
#ifndef OXIDE_TARGET_ABI_ILP32
    #define OXIDE_TARGET_ABI_ILP32 0
#endif
#ifndef OXIDE_TARGET_ABI_MACABI
    #define OXIDE_TARGET_ABI_MACABI 0
#endif
#ifndef OXIDE_TARGET_ABI_SIM
    #define OXIDE_TARGET_ABI_SIM 0
#endif
#ifndef OXIDE_TARGET_ABI_SOFTFLOAT
    #define OXIDE_TARGET_ABI_SOFTFLOAT 0
#endif
#ifndef OXIDE_TARGET_ABI_LLVM
    #define OXIDE_TARGET_ABI_LLVM 0
#endif
#ifndef OXIDE_TARGET_ABI_UWP
    #define OXIDE_TARGET_ABI_UWP 0
#endif
#ifndef OXIDE_TARGET_ABI_NONE
    #define OXIDE_TARGET_ABI_NONE 0
#endif

#if (OXIDE_TARGET_ABI_EABI != 0 && OXIDE_TARGET_ABI_EABI != 1) || \
    (OXIDE_TARGET_ABI_EABIHF != 0 && OXIDE_TARGET_ABI_EABIHF != 1) || \
    (OXIDE_TARGET_ABI_X32 != 0 && OXIDE_TARGET_ABI_X32 != 1) || \
    (OXIDE_TARGET_ABI_ILP32 != 0 && OXIDE_TARGET_ABI_ILP32 != 1) || \
    (OXIDE_TARGET_ABI_MACABI != 0 && OXIDE_TARGET_ABI_MACABI != 1) || \
    (OXIDE_TARGET_ABI_SIM != 0 && OXIDE_TARGET_ABI_SIM != 1) || \
    (OXIDE_TARGET_ABI_SOFTFLOAT != 0 && OXIDE_TARGET_ABI_SOFTFLOAT != 1) || \
    (OXIDE_TARGET_ABI_LLVM != 0 && OXIDE_TARGET_ABI_LLVM != 1) || \
    (OXIDE_TARGET_ABI_UWP != 0 && OXIDE_TARGET_ABI_UWP != 1) || \
    (OXIDE_TARGET_ABI_NONE != 0 && OXIDE_TARGET_ABI_NONE != 1) || \
    (OXIDE_TARGET_ABI_EABI + OXIDE_TARGET_ABI_EABIHF + OXIDE_TARGET_ABI_X32 + \
     OXIDE_TARGET_ABI_ILP32 + OXIDE_TARGET_ABI_MACABI + OXIDE_TARGET_ABI_SIM + \
     OXIDE_TARGET_ABI_SOFTFLOAT + OXIDE_TARGET_ABI_LLVM + OXIDE_TARGET_ABI_UWP + \
     OXIDE_TARGET_ABI_NONE) != 1
    #error "Exactly one target ABI flag must be 1; all others must be 0"
#endif

#if !defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__) || !defined(__ORDER_BIG_ENDIAN__)
    #if OXIDE_TARGET_OS_WINDOWS
        #define OXIDE_TARGET_LITTLE_ENDIAN 1
    #else
        #error "Unsupported compiler: cannot determine target endianness"
    #endif
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define OXIDE_TARGET_LITTLE_ENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define OXIDE_TARGET_LITTLE_ENDIAN 0
#else
    #error "Unsupported target endianness"
#endif
// clang-format on

namespace oxide::target {

enum class Arch {
    X86,
    X86_64,
    ARM,
    AARCH64,
    RISCV32,
    RISCV64,
};

namespace arch {

inline constexpr bool X86 = OXIDE_TARGET_ARCH_X86;
inline constexpr bool X86_64 = OXIDE_TARGET_ARCH_X86_64;
inline constexpr bool ARM = OXIDE_TARGET_ARCH_ARM;
inline constexpr bool AARCH64 = OXIDE_TARGET_ARCH_AARCH64;
inline constexpr bool RISCV32 = OXIDE_TARGET_ARCH_RISCV32;
inline constexpr bool RISCV64 = OXIDE_TARGET_ARCH_RISCV64;

inline constexpr Arch NATIVE = // clang-format off
    #if OXIDE_TARGET_ARCH_X86_64
        Arch::X86_64;
    #elif OXIDE_TARGET_ARCH_X86
        Arch::X86;
    #elif OXIDE_TARGET_ARCH_AARCH64
        Arch::AARCH64;
    #elif OXIDE_TARGET_ARCH_ARM
        Arch::ARM;
    #elif OXIDE_TARGET_ARCH_RISCV64
        Arch::RISCV64;
    #else
        Arch::RISCV32;
    #endif // clang-format on

} // namespace arch

enum class Os {
    WINDOWS,
    MACOS,
    IOS,
    TVOS,
    WATCHOS,
    VISIONOS,
    LINUX,
    ANDROID,
    FREEBSD,
    DRAGONFLY,
    OPENBSD,
    NETBSD,
    FUCHSIA,
    NONE,
};

namespace os {

inline constexpr bool WINDOWS = OXIDE_TARGET_OS_WINDOWS;
inline constexpr bool MACOS = OXIDE_TARGET_OS_MACOS;
inline constexpr bool IOS = OXIDE_TARGET_OS_IOS;
inline constexpr bool TVOS = OXIDE_TARGET_OS_TVOS;
inline constexpr bool WATCHOS = OXIDE_TARGET_OS_WATCHOS;
inline constexpr bool VISIONOS = OXIDE_TARGET_OS_VISIONOS;
inline constexpr bool LINUX = OXIDE_TARGET_OS_LINUX;
inline constexpr bool ANDROID = OXIDE_TARGET_OS_ANDROID;
inline constexpr bool FREEBSD = OXIDE_TARGET_OS_FREEBSD;
inline constexpr bool DRAGONFLY = OXIDE_TARGET_OS_DRAGONFLY;
inline constexpr bool OPENBSD = OXIDE_TARGET_OS_OPENBSD;
inline constexpr bool NETBSD = OXIDE_TARGET_OS_NETBSD;
inline constexpr bool FUCHSIA = OXIDE_TARGET_OS_FUCHSIA;
inline constexpr bool NONE = OXIDE_TARGET_OS_NONE;

inline constexpr Os NATIVE = // clang-format off
    #if OXIDE_TARGET_OS_WINDOWS
        Os::WINDOWS;
    #elif OXIDE_TARGET_OS_MACOS
        Os::MACOS;
    #elif OXIDE_TARGET_OS_IOS
        Os::IOS;
    #elif OXIDE_TARGET_OS_TVOS
        Os::TVOS;
    #elif OXIDE_TARGET_OS_WATCHOS
        Os::WATCHOS;
    #elif OXIDE_TARGET_OS_VISIONOS
        Os::VISIONOS;
    #elif OXIDE_TARGET_OS_LINUX
        Os::LINUX;
    #elif OXIDE_TARGET_OS_ANDROID
        Os::ANDROID;
    #elif OXIDE_TARGET_OS_FREEBSD
        Os::FREEBSD;
    #elif OXIDE_TARGET_OS_DRAGONFLY
        Os::DRAGONFLY;
    #elif OXIDE_TARGET_OS_OPENBSD
        Os::OPENBSD;
    #elif OXIDE_TARGET_OS_NETBSD
        Os::NETBSD;
    #elif OXIDE_TARGET_OS_FUCHSIA
        Os::FUCHSIA;
    #else
        Os::NONE;
    #endif // clang-format on

} // namespace os

enum class Env {
    GNU,
    MSVC,
    MUSL,
    NONE,
};

namespace env {

inline constexpr bool GNU = OXIDE_TARGET_ENV_GNU;
inline constexpr bool MSVC = OXIDE_TARGET_ENV_MSVC;
inline constexpr bool MUSL = OXIDE_TARGET_ENV_MUSL;
inline constexpr bool NONE = OXIDE_TARGET_ENV_NONE;

inline constexpr Env NATIVE = // clang-format off
    #if OXIDE_TARGET_ENV_GNU
        Env::GNU;
    #elif OXIDE_TARGET_ENV_MSVC
        Env::MSVC;
    #elif OXIDE_TARGET_ENV_MUSL
        Env::MUSL;
    #else
        Env::NONE;
    #endif // clang-format on

} // namespace env

enum class Abi {
    EABI,
    EABIHF,
    X32,
    ILP32,
    MACABI,
    SIM,
    SOFTFLOAT,
    LLVM,
    UWP,
    NONE,
};

namespace abi {

inline constexpr bool EABI = OXIDE_TARGET_ABI_EABI;
inline constexpr bool EABIHF = OXIDE_TARGET_ABI_EABIHF;
inline constexpr bool X32 = OXIDE_TARGET_ABI_X32;
inline constexpr bool ILP32 = OXIDE_TARGET_ABI_ILP32;
inline constexpr bool MACABI = OXIDE_TARGET_ABI_MACABI;
inline constexpr bool SIM = OXIDE_TARGET_ABI_SIM;
inline constexpr bool SOFTFLOAT = OXIDE_TARGET_ABI_SOFTFLOAT;
inline constexpr bool LLVM = OXIDE_TARGET_ABI_LLVM;
inline constexpr bool UWP = OXIDE_TARGET_ABI_UWP;
inline constexpr bool NONE = OXIDE_TARGET_ABI_NONE;

inline constexpr Abi NATIVE = // clang-format off
    #if OXIDE_TARGET_ABI_EABI
        Abi::EABI;
    #elif OXIDE_TARGET_ABI_EABIHF
        Abi::EABIHF;
    #elif OXIDE_TARGET_ABI_X32
        Abi::X32;
    #elif OXIDE_TARGET_ABI_ILP32
        Abi::ILP32;
    #elif OXIDE_TARGET_ABI_MACABI
        Abi::MACABI;
    #elif OXIDE_TARGET_ABI_SIM
        Abi::SIM;
    #elif OXIDE_TARGET_ABI_SOFTFLOAT
        Abi::SOFTFLOAT;
    #elif OXIDE_TARGET_ABI_LLVM
        Abi::LLVM;
    #elif OXIDE_TARGET_ABI_UWP
        Abi::UWP;
    #else
        Abi::NONE;
    #endif // clang-format on

} // namespace abi

enum class Endian : unsigned char {
    LITTLE = 0,
    BIG,
};

namespace endian {

inline constexpr bool LITTLE = OXIDE_TARGET_LITTLE_ENDIAN;
inline constexpr bool BIG = !LITTLE;

inline constexpr Endian NATIVE = LITTLE ? Endian::LITTLE : Endian::BIG;

} // namespace endian

inline constexpr unsigned POINTER_WIDTH = OXIDE_TARGET_POINTER_WIDTH;

} // namespace oxide::target

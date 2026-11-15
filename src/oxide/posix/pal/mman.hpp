#pragma once

#include <sys/mman.h>

#include <oxide/log/logger.hpp>
#include <oxide/types.hpp>
#include <oxide/posix/pal/errno.hpp>
#include <oxide/posix/pal/fcntl.hpp>

#if OXIDE_TARGET_OS_MACOS // clang-format off
    #include <oxide/posix/darwin/mman.hpp>
#endif // clang-format on

namespace oxide::posix {

class MappingProtection final {
public:
    enum struct Flags : i32 {
        NONE = 0,            // No permissions
        READ = PROT_READ,    // Pages can be read
        WRITE = PROT_WRITE,  // Pages can be written
        EXECUTE = PROT_EXEC, // Pages can be executed
        MASK = PROT_READ | PROT_WRITE | PROT_EXEC,
    };

    OXIDE_REEXPORT_FLAGS(NONE);
    OXIDE_REEXPORT_FLAGS(READ);
    OXIDE_REEXPORT_FLAGS(WRITE);
    OXIDE_REEXPORT_FLAGS(EXECUTE);
    OXIDE_REEXPORT_FLAGS(MASK);

    constexpr MappingProtection() noexcept = default;

    constexpr MappingProtection(Flags flags) noexcept : _flags(flags) {}

    template <typename... Modes>
    constexpr MappingProtection(Flags first, Modes... rest) noexcept :
        _flags((first | ... | rest)) {}

    static constexpr MappingProtection from(i32 flags) noexcept {
        i32 mask = static_cast<i32>(MASK);
        return MappingProtection{static_cast<Flags>(flags & mask)};
    }

    static constexpr MappingProtection from(AccessMode access) noexcept;

    static constexpr MappingProtection none() noexcept { return MappingProtection{NONE}; }

    constexpr Flags flags() const noexcept { return _flags; }

    constexpr i32 bits() const noexcept { return static_cast<i32>(_flags); }

    [[nodiscard]] constexpr bool contains(Flags flags) const noexcept;

    [[nodiscard]] constexpr MappingProtection operator|(Flags flags) const noexcept;
    [[nodiscard]] constexpr MappingProtection operator&(Flags mask) const noexcept;
    constexpr MappingProtection& operator|=(Flags flags) noexcept;
    constexpr MappingProtection& operator&=(Flags mask) noexcept;

    [[nodiscard]] constexpr MappingProtection operator|(MappingProtection other) const noexcept;
    constexpr MappingProtection& operator|=(MappingProtection other) noexcept;

private:
    Flags _flags{NONE};
};

OXIDE_ENABLE_BITMASK_OPERATORS(MappingProtection::Flags)

constexpr MappingProtection MappingProtection::from(AccessMode access) noexcept {
    switch (access.flags()) { // clang-format off
        case AccessMode::READ: return MappingProtection{READ};
        case AccessMode::WRITE: return MappingProtection{WRITE};
        case AccessMode::READ_WRITE: return MappingProtection{READ | WRITE};
        default: return MappingProtection{NONE};
    } // clang-format on
}

constexpr bool MappingProtection::contains(Flags flags) const noexcept {
    return (_flags & flags) == flags;
}

constexpr MappingProtection MappingProtection::operator|(Flags flags) const noexcept {
    return MappingProtection(_flags | flags);
}

constexpr MappingProtection MappingProtection::operator&(Flags mask) const noexcept {
    return MappingProtection(_flags & mask);
}

constexpr MappingProtection& MappingProtection::operator|=(Flags flags) noexcept {
    _flags |= flags;
    return *this;
}

constexpr MappingProtection& MappingProtection::operator&=(Flags mask) noexcept {
    _flags &= mask;
    return *this;
}

constexpr MappingProtection MappingProtection::operator|(MappingProtection other) const noexcept {
    return MappingProtection(_flags | other._flags);
}

constexpr MappingProtection& MappingProtection::operator|=(MappingProtection other) noexcept {
    _flags |= other._flags;
    return *this;
}

class MappingBehavior final {
public:
    enum struct Flags : i32 {
        SHARED = MAP_SHARED,   // Share changes
        PRIVATE = MAP_PRIVATE, // Changes are private
    };

    OXIDE_REEXPORT_FLAGS(SHARED);
    OXIDE_REEXPORT_FLAGS(PRIVATE);

    constexpr MappingBehavior() = default;

    constexpr MappingBehavior(Flags flags) : _flags(flags) {}

    constexpr Flags flags() const { return _flags; }

    constexpr i32 bits() const { return static_cast<i32>(_flags); }

private:
    Flags _flags{PRIVATE};
};

class MappingFlags final {
public:
    enum struct Flags : i32 {
        NONE = 0,
        ANONYMOUS = MAP_ANONYMOUS,
        FIXED = MAP_FIXED,
    };

    OXIDE_REEXPORT_FLAGS(NONE);
    OXIDE_REEXPORT_FLAGS(ANONYMOUS);
    OXIDE_REEXPORT_FLAGS(FIXED);

    constexpr MappingFlags() = default;

    constexpr MappingFlags(Flags flags) : _flags(flags) {}

    template <typename... Args>
    constexpr MappingFlags(Flags first, Args... rest) noexcept : _flags((first | ... | rest)) {}

    static constexpr MappingFlags none() noexcept { return MappingFlags{NONE}; }
    static constexpr MappingFlags anonymous() noexcept { return MappingFlags{ANONYMOUS}; }
    static constexpr MappingFlags fixed() noexcept { return MappingFlags{FIXED}; }

    constexpr Flags flags() const noexcept { return _flags; }

    constexpr i32 bits() const noexcept { return static_cast<i32>(_flags); }

    [[nodiscard]] constexpr bool contains(Flags flags) const noexcept;

    [[nodiscard]] constexpr MappingFlags operator|(Flags flags) const noexcept;
    [[nodiscard]] constexpr MappingFlags operator&(Flags mask) const noexcept;
    constexpr MappingFlags& operator|=(Flags flags) noexcept;
    constexpr MappingFlags& operator&=(Flags mask) noexcept;

    [[nodiscard]] constexpr MappingFlags operator|(MappingFlags other) const noexcept;
    constexpr MappingFlags& operator|=(MappingFlags other) noexcept;

private:
    Flags _flags{NONE};
};

OXIDE_ENABLE_BITMASK_OPERATORS(MappingFlags::Flags)

constexpr bool MappingFlags::contains(Flags flags) const noexcept {
    return (_flags & flags) == flags;
}

constexpr MappingFlags MappingFlags::operator|(Flags flags) const noexcept {
    return MappingFlags(_flags | flags);
}

constexpr MappingFlags MappingFlags::operator&(Flags mask) const noexcept {
    return MappingFlags(_flags & mask);
}

constexpr MappingFlags& MappingFlags::operator|=(Flags flags) noexcept {
    _flags |= flags;
    return *this;
}

constexpr MappingFlags& MappingFlags::operator&=(Flags mask) noexcept {
    _flags &= mask;
    return *this;
}

constexpr MappingFlags MappingFlags::operator|(MappingFlags other) const noexcept {
    return MappingFlags(_flags | other._flags);
}

constexpr MappingFlags& MappingFlags::operator|=(MappingFlags other) noexcept {
    _flags |= other._flags;
    return *this;
}

class LockMode final {
public:
    enum struct Flags : i32 {
        NONE = 0,

        CURRENT = MCL_CURRENT, // Lock all currently mapped pages
        FUTURE = MCL_FUTURE,   // Lock pages mapped in the future

        // clang-format off
        #if OXIDE_TARGET_OS_LINUX
            ON_FAULT = MCL_ONFAULT, // Lock pages only after they are faulted in
        #endif
        MASK = MCL_CURRENT | MCL_FUTURE
        #if OXIDE_TARGET_OS_LINUX
            | MCL_ONFAULT
        #endif
        // clang-format on
    };

    OXIDE_REEXPORT_FLAGS(NONE);
    OXIDE_REEXPORT_FLAGS(CURRENT);
    OXIDE_REEXPORT_FLAGS(FUTURE); // clang-format off
    #if OXIDE_TARGET_OS_LINUX
        OXIDE_REEXPORT_FLAGS(ON_FAULT);
    #endif // clang-format on
    OXIDE_REEXPORT_FLAGS(MASK);

    constexpr LockMode() = default;

    constexpr LockMode(Flags flags) : _flags(flags) {}

    constexpr i32 bits() const { return static_cast<i32>(_flags); }

private:
    Flags _flags{NONE};
};

inline i32 mlock(const void* addr, usize len) {
    return ::mlock(addr, len);
}

inline i32 munlock(const void* addr, usize len) {
    return ::munlock(addr, len);
}

inline i32 mlockall(LockMode mode) {
    return ::mlockall(mode.bits());
}

inline i32 munlockall() {
    return ::munlockall();
}

inline void* mmap(
    void* addr,
    usize len,
    MappingProtection protection,
    MappingFlags flags,
    MappingBehavior behavior,
    RawFd fd,
    usize offset
) {
    return ::mmap(
        addr,
        len,
        protection.bits(),
        flags.bits() | behavior.bits(),
        fd,
        static_cast<off_t>(offset)
    );
}

inline i32 mprotect(void* addr, usize len, MappingProtection protection) noexcept {
    return ::mprotect(addr, len, protection.bits());
}

inline i32 munmap(void* addr, usize len) noexcept {
    return ::munmap(addr, len);
}

inline OwnedFd shm_open(const CStr& name, OFlag flags, Mode mode) {
    // clang-format off
    #if OXIDE_TARGET_OS_MACOS
        auto native = darwin::shm_native_name(name);
        if (native.is_none()) {
            return OwnedFd::invalid();
        }

        const i32 raw_fd = ::shm_open(native->data(), flags.bits(), mode.bits());
        return OwnedFd::unsafe::from_raw_fd(RawFd{raw_fd});
    #elif OXIDE_TARGET_OS_LINUX
        const i32 raw_fd = ::shm_open(name.data(), flags.bits(), mode.bits());
        return OwnedFd::unsafe::from_raw_fd(RawFd{raw_fd});
    #endif // clang-format on
}

inline i32 shm_unlink(const CStr& name) {
    // clang-format off
    #if OXIDE_TARGET_OS_MACOS
        auto native = darwin::shm_native_name(name);
        if (native.is_none()) {
            return -1;
        }

        return ::shm_unlink(native->data());
    #elif OXIDE_TARGET_OS_LINUX
        return ::shm_unlink(name.data());
    #endif // clang-format on
}

} // namespace oxide::posix

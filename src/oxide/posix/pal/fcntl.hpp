#pragma once

#include <fcntl.h>

#include <sys/stat.h>

#include <oxide/compiler/macros.hpp>
#include <oxide/bits.hpp>
#include <oxide/types.hpp>
#include <oxide/posix/pal/fd.hpp>

namespace oxide::posix {

class Mode final {
public:
    enum struct Flags : mode_t {
        NONE = 0,

        OWNER_READ = S_IRUSR,
        OWNER_WRITE = S_IWUSR,
        OWNER_EXECUTE = S_IXUSR,
        OWNER_ALL = S_IRUSR | S_IWUSR | S_IXUSR,

        GROUP_READ = S_IRGRP,
        GROUP_WRITE = S_IWGRP,
        GROUP_EXECUTE = S_IXGRP,
        GROUP_ALL = S_IRGRP | S_IWGRP | S_IXGRP,

        OTHER_READ = S_IROTH,
        OTHER_WRITE = S_IWOTH,
        OTHER_EXECUTE = S_IXOTH,
        OTHER_ALL = S_IROTH | S_IWOTH | S_IXOTH,

        ALL = S_IRWXU | S_IRWXG | S_IRWXO,

        SET_UID = S_ISUID,
        SET_GID = S_ISGID,
        STICKY = S_ISVTX,

        MASK = S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO,
    };

    OXIDE_REEXPORT_FLAGS(NONE);
    OXIDE_REEXPORT_FLAGS(OWNER_READ);
    OXIDE_REEXPORT_FLAGS(OWNER_WRITE);
    OXIDE_REEXPORT_FLAGS(OWNER_EXECUTE);
    OXIDE_REEXPORT_FLAGS(OWNER_ALL);
    OXIDE_REEXPORT_FLAGS(GROUP_READ);
    OXIDE_REEXPORT_FLAGS(GROUP_WRITE);
    OXIDE_REEXPORT_FLAGS(GROUP_EXECUTE);
    OXIDE_REEXPORT_FLAGS(GROUP_ALL);
    OXIDE_REEXPORT_FLAGS(OTHER_READ);
    OXIDE_REEXPORT_FLAGS(OTHER_WRITE);
    OXIDE_REEXPORT_FLAGS(OTHER_EXECUTE);
    OXIDE_REEXPORT_FLAGS(OTHER_ALL);
    OXIDE_REEXPORT_FLAGS(ALL);
    OXIDE_REEXPORT_FLAGS(SET_UID);
    OXIDE_REEXPORT_FLAGS(SET_GID);
    OXIDE_REEXPORT_FLAGS(STICKY);
    OXIDE_REEXPORT_FLAGS(MASK);

    constexpr Mode() noexcept = default;

    constexpr Mode(Flags flags) noexcept : _flags(flags) {}

    constexpr explicit Mode(mode_t mode) noexcept :
        _flags(static_cast<Flags>(mode & static_cast<mode_t>(MASK))) {}

    template <typename... Args>
    constexpr Mode(Flags first, Args... rest) noexcept : _flags((first | ... | rest)) {}

    static constexpr Mode none() noexcept { return Mode{NONE}; }

    constexpr Flags flags() const noexcept { return _flags; }

    constexpr i32 bits() const noexcept { return static_cast<i32>(_flags); }

    [[nodiscard]] constexpr bool contains(Flags flags) const noexcept;

    [[nodiscard]] constexpr Mode operator|(Flags flags) const noexcept;
    [[nodiscard]] constexpr Mode operator&(Flags mask) const noexcept;
    constexpr Mode& operator|=(Flags flags) noexcept;
    constexpr Mode& operator&=(Flags mask) noexcept;

    [[nodiscard]] constexpr Mode operator|(Mode other) const noexcept;
    constexpr Mode& operator|=(Mode other) noexcept;

private:
    Flags _flags{NONE};
};

OXIDE_ENABLE_BITMASK_OPERATORS(Mode::Flags)

constexpr bool Mode::contains(Flags flags) const noexcept {
    return (_flags & flags) == flags;
}

constexpr Mode Mode::operator|(Flags flags) const noexcept {
    return Mode(_flags | flags);
}

constexpr Mode Mode::operator&(Flags mask) const noexcept {
    return Mode(_flags & mask);
}

constexpr Mode& Mode::operator|=(Flags flags) noexcept {
    _flags |= flags;
    return *this;
}

constexpr Mode& Mode::operator&=(Flags mask) noexcept {
    _flags &= mask;
    return *this;
}

constexpr Mode Mode::operator|(Mode other) const noexcept {
    return Mode(_flags | other._flags);
}

constexpr Mode& Mode::operator|=(Mode other) noexcept {
    _flags |= other._flags;
    return *this;
}

class OFlag final {
public:
    enum struct Flags : i32 {
        READ_ONLY = O_RDONLY,
        WRITE_ONLY = O_WRONLY,
        READ_WRITE = O_RDWR,

        CREATE = O_CREAT,
        TRUNCATE = O_TRUNC,
        EXCLUSIVE = O_EXCL,
        APPEND = O_APPEND,
        NON_BLOCKING = O_NONBLOCK,
        CLOSE_ON_EXEC = O_CLOEXEC,
        NO_FOLLOW = O_NOFOLLOW,
        SYNC = O_SYNC,

        // clang-format off
        #if OXIDE_TARGET_OS_MACOS
            SHARED_LOCK = O_SHLOCK,
            EXCLUSIVE_LOCK = O_EXLOCK,
            ASYNC = O_ASYNC,
            RESOLVE_BENEATH = O_RESOLVE_BENEATH,
            UNIQUE = O_UNIQUE,
        #endif // clang-format on
    };

    OXIDE_REEXPORT_FLAGS(READ_ONLY);
    OXIDE_REEXPORT_FLAGS(WRITE_ONLY);
    OXIDE_REEXPORT_FLAGS(READ_WRITE);
    OXIDE_REEXPORT_FLAGS(CREATE);
    OXIDE_REEXPORT_FLAGS(TRUNCATE);
    OXIDE_REEXPORT_FLAGS(EXCLUSIVE);
    OXIDE_REEXPORT_FLAGS(APPEND);
    OXIDE_REEXPORT_FLAGS(NON_BLOCKING);
    OXIDE_REEXPORT_FLAGS(CLOSE_ON_EXEC);
    OXIDE_REEXPORT_FLAGS(NO_FOLLOW);
    OXIDE_REEXPORT_FLAGS(SYNC);
    // clang-format off
    #if OXIDE_TARGET_OS_MACOS
        OXIDE_REEXPORT_FLAGS(SHARED_LOCK);
        OXIDE_REEXPORT_FLAGS(EXCLUSIVE_LOCK);
        OXIDE_REEXPORT_FLAGS(ASYNC);
        OXIDE_REEXPORT_FLAGS(RESOLVE_BENEATH);
        OXIDE_REEXPORT_FLAGS(UNIQUE);
    #endif // clang-format on

    constexpr OFlag(Flags flags) noexcept : _flags(flags) {}

    constexpr explicit OFlag(i32 flags) noexcept : _flags(static_cast<Flags>(flags)) {}

    template <typename... FlagsT>
    constexpr OFlag(Flags first, FlagsT... rest) noexcept : _flags((first | ... | rest)) {}

    constexpr Flags flags() const noexcept { return _flags; }

    constexpr i32 bits() const noexcept { return static_cast<i32>(_flags); }

    [[nodiscard]] constexpr bool contains(Flags flags) const noexcept;
    [[nodiscard]] constexpr bool is_read_only() const noexcept;
    [[nodiscard]] constexpr bool is_write_only() const noexcept;
    [[nodiscard]] constexpr bool is_read_write() const noexcept;

    [[nodiscard]] constexpr OFlag operator|(Flags flags) const noexcept;
    [[nodiscard]] constexpr OFlag operator&(Flags mask) const noexcept;
    constexpr OFlag& operator|=(Flags flags) noexcept;
    constexpr OFlag& operator&=(Flags mask) noexcept;

    [[nodiscard]] constexpr OFlag operator|(OFlag other) const noexcept;
    constexpr OFlag& operator|=(OFlag other) noexcept;

private:
    Flags _flags{READ_ONLY};
};

OXIDE_ENABLE_BITMASK_OPERATORS(OFlag::Flags)

constexpr bool OFlag::contains(Flags flags) const noexcept {
    return (_flags & flags) == flags;
}

constexpr bool OFlag::is_read_only() const noexcept {
    return (bits() & O_ACCMODE) == O_RDONLY;
}

constexpr bool OFlag::is_write_only() const noexcept {
    return (bits() & O_ACCMODE) == O_WRONLY;
}

constexpr bool OFlag::is_read_write() const noexcept {
    return (bits() & O_ACCMODE) == O_RDWR;
}

constexpr OFlag OFlag::operator|(Flags flags) const noexcept {
    return OFlag(_flags | flags);
}

constexpr OFlag OFlag::operator&(Flags mask) const noexcept {
    return OFlag(_flags & mask);
}

constexpr OFlag& OFlag::operator|=(Flags flags) noexcept {
    _flags |= flags;
    return *this;
}

constexpr OFlag& OFlag::operator&=(Flags mask) noexcept {
    _flags &= mask;
    return *this;
}

constexpr OFlag OFlag::operator|(OFlag other) const noexcept {
    return OFlag(_flags | other._flags);
}

constexpr OFlag& OFlag::operator|=(OFlag other) noexcept {
    _flags |= other._flags;
    return *this;
}

class AccessMode final {
public:
    enum struct Flags : i32 {
        READ = O_RDONLY,
        WRITE = O_WRONLY,
        READ_WRITE = O_RDWR,
    };

    OXIDE_REEXPORT_FLAGS(READ);
    OXIDE_REEXPORT_FLAGS(WRITE);
    OXIDE_REEXPORT_FLAGS(READ_WRITE);

    constexpr AccessMode() noexcept : _flags(READ) {}

    constexpr AccessMode(Flags flags) noexcept : _flags(flags) {}

    constexpr explicit AccessMode(i32 flags) noexcept : _flags(static_cast<Flags>(flags)) {}

    template <typename... Modes>
    constexpr AccessMode(Flags first, Modes... rest) noexcept : _flags((first | ... | rest)) {}

    static constexpr AccessMode read() noexcept { return AccessMode{READ}; }

    static constexpr AccessMode write() noexcept { return AccessMode{WRITE}; }

    static constexpr AccessMode read_write() noexcept { return AccessMode{READ_WRITE}; }

    constexpr Flags flags() const noexcept { return _flags; }

    constexpr i32 bits() const noexcept { return static_cast<i32>(_flags); }

    constexpr OFlag oflags() const noexcept { return OFlag(bits()); }

    [[nodiscard]] constexpr bool contains(Flags flags) const noexcept;

    [[nodiscard]] constexpr bool has_read() const noexcept {
        return _flags == READ || _flags == READ_WRITE;
    }

    [[nodiscard]] constexpr AccessMode operator|(Flags flags) const noexcept;

    [[nodiscard]] constexpr AccessMode operator&(Flags mask) const noexcept;

    constexpr AccessMode& operator|=(Flags flags) noexcept;

    constexpr AccessMode& operator&=(Flags mask) noexcept;

private:
    Flags _flags{READ};
};

OXIDE_ENABLE_BITMASK_OPERATORS(AccessMode::Flags)

constexpr bool AccessMode::contains(Flags flags) const noexcept {
    return (_flags & flags) == flags;
}

constexpr AccessMode AccessMode::operator|(Flags flags) const noexcept {
    return AccessMode(_flags | flags);
}

constexpr AccessMode AccessMode::operator&(Flags mask) const noexcept {
    return AccessMode(_flags & mask);
}

constexpr AccessMode& AccessMode::operator|=(Flags flags) noexcept {
    _flags |= flags;
    return *this;
}

constexpr AccessMode& AccessMode::operator&=(Flags mask) noexcept {
    _flags &= mask;
    return *this;
}

[[nodiscard]] inline OwnedFd open(const CStr& path, OFlag oflags) noexcept {
    return OwnedFd::unsafe::from_raw_fd(RawFd{::open(path.data(), oflags.bits())});
}

[[nodiscard]] inline OwnedFd open(const CStr& path, OFlag oflags, Mode mode) noexcept {
    return OwnedFd::unsafe::from_raw_fd(RawFd{::open(path.data(), oflags.bits(), mode.bits())});
}

[[nodiscard]] inline i32 fcntl(RawFd fd, i32 command) noexcept {
    return ::fcntl(fd, command);
}

[[nodiscard]] inline i32 fcntl(RawFd fd, i32 command, i32 value) noexcept {
    return ::fcntl(fd, command, value);
}

[[nodiscard]] inline i32 fcntl(RawFd fd, i32 command, struct flock* lock) noexcept {
    return ::fcntl(fd, command, lock);
}

} // namespace oxide::posix

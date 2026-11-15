#pragma once

#include <sys/mman.h>

#include <utility>

#include <oxide/c_string.hpp>
#include <oxide/types.hpp>
#include <oxide/assert.hpp>
#include <oxide/fmt/format.hpp>
#include <oxide/posix/system.hpp>
#include <oxide/posix/pal/errno.hpp>
#include <oxide/posix/pal/fcntl.hpp>
#include <oxide/posix/pal/mman.hpp>

namespace oxide::posix {

class MemoryLock final {
public:
    template <class T>
    using Result = oxide::Result<T, Errno>;

    static Result<MemoryLock> create(const void* address, usize size) {
        if (posix::mlock(address, size) == -1) {
            return Err{Errno::last()};
        }
        return Ok{MemoryLock{address, size}};
    }

    static Result<Empty> lock_all(LockMode mode) {
        if (posix::mlockall(mode) == -1) {
            return Err{Errno::last()};
        }
        return Ok{Empty{}};
    }

    static Result<Empty> unlock_all() {
        if (posix::munlockall() == -1) {
            return Err{Errno::last()};
        }
        return Ok{Empty{}};
    }

    MemoryLock(MemoryLock&& other) noexcept { move_from(std::move(other)); }

    MemoryLock& operator=(MemoryLock&& other) noexcept {
        if (this != &other) {
            reset();
            move_from(std::move(other));
        }
        return *this;
    }

    OXIDE_DISABLE_COPY(MemoryLock);

    ~MemoryLock() noexcept { reset(); }

private:
    const void* _address{nullptr};
    usize _size{0};

    MemoryLock(const void* address, usize size) : _address(address), _size(size) {}

    void reset() noexcept {
        const int saved_errno = errno;
        if (_address != nullptr) {
            OXIDE_ASSERT(posix::munlock(_address, _size) == 0, "Unable to unlock memory region.");
            _address = nullptr;
            _size = 0;
        }
        errno = saved_errno;
    }

    void move_from(MemoryLock&& other) noexcept {
        _address = std::exchange(other._address, nullptr);
        _size = std::exchange(other._size, 0);
    }
};

struct MemoryMappingCreationError final {
    enum struct Error {
        INVALID_SIZE,
        FAILED_TO_ENFORCE_ADDRESS_HINT,
        BROKEN_FILE_DESCRIPTOR,
        ERRNO,
    };

    OXIDE_REEXPORT_ERROR(INVALID_SIZE);
    OXIDE_REEXPORT_ERROR(FAILED_TO_ENFORCE_ADDRESS_HINT);
    OXIDE_REEXPORT_ERROR(BROKEN_FILE_DESCRIPTOR);
    OXIDE_REEXPORT_ERROR(ERRNO);

    Error error;
    i32 code{0};

    MemoryMappingCreationError(Error error) : error(error) {}

    static MemoryMappingCreationError from_errno() {
        return MemoryMappingCreationError::from_errno(Errno::last());
    }

    static MemoryMappingCreationError from_errno(Errno error) { return {ERRNO, error.code()}; }

    static MemoryMappingCreationError broken_file_descriptor() {
        return broken_file_descriptor(Errno::last());
    }

    static MemoryMappingCreationError broken_file_descriptor(Errno error) {
        return {BROKEN_FILE_DESCRIPTOR, error.code()};
    }

    Errno to_errno() const noexcept {
        switch (error) {
            case INVALID_SIZE:
                return Errno::INVALID_ARGUMENT;
            case FAILED_TO_ENFORCE_ADDRESS_HINT:
                return Errno::ADDRESS_NOT_AVAILABLE;
            case BROKEN_FILE_DESCRIPTOR:
                return Errno::BAD_FD;
            case ERRNO:
                return Errno{code};
        }
        OXIDE_UNREACHABLE();
    }

    String to_string() const {
        const char* e;
        switch (error) { // clang-format off
            case BROKEN_FILE_DESCRIPTOR: e = "BrokenFileDescriptor"; break;
            case ERRNO: e = "Errno"; break;
            case INVALID_SIZE: e = "InvalidSize"; break;
            case FAILED_TO_ENFORCE_ADDRESS_HINT: e = "FailedToEnforceAddressHint"; break;
            default: e = "UnknownError"; break;
        } // clang-format on

        return fmt::format("{}({})", e, code);
    }

private:
    MemoryMappingCreationError(Error error, i32 code) : error(error), code(code) {}
};

struct MemoryProtectionError final {
    enum struct Error {
        INVALID_SIZE,
        NOT_ALIGNED,
        INVALID_RANGE,
        INSUFFICIENT_PERMISSIONS,
        INSUFFICIENT_MEMORY,
        UNSUPPORTED_PERMISSION,
        UNKNOWN_FAILURE,
    };

    OXIDE_REEXPORT_ERROR(INVALID_SIZE);
    OXIDE_REEXPORT_ERROR(NOT_ALIGNED);
    OXIDE_REEXPORT_ERROR(INVALID_RANGE);
    OXIDE_REEXPORT_ERROR(INSUFFICIENT_PERMISSIONS);
    OXIDE_REEXPORT_ERROR(INSUFFICIENT_MEMORY);
    OXIDE_REEXPORT_ERROR(UNSUPPORTED_PERMISSION);
    OXIDE_REEXPORT_ERROR(UNKNOWN_FAILURE);

    Error error{UNKNOWN_FAILURE};
    i32 code{0};

    MemoryProtectionError(Error error) : error(error) {}

    static MemoryProtectionError from_errno() {
        return MemoryProtectionError::from_errno(Errno::last());
    }

    static MemoryProtectionError from_errno(Errno error) {
        switch (error.match()) { // clang-format off
            case Errno::ACCESS: return {INSUFFICIENT_PERMISSIONS, error.code()};
            case Errno::TRY_AGAIN: return {INSUFFICIENT_MEMORY, error.code()};
            case Errno::NO_MEMORY: return {INSUFFICIENT_MEMORY, error.code()};
            #ifdef ENOTSUP
                case Errno::NOT_SUPPORTED: return {UNSUPPORTED_PERMISSION, error.code()};
            #endif
            default: return {UNKNOWN_FAILURE, error.code()};
        } // clang-format on
    }

private:
    MemoryProtectionError(Error error, i32 code) : error(error), code(code) {}
};

struct MemoryMappingSettings {
    // Access permissions for the mapped memory.
    MappingProtection protection;
    // Shared or private mapping behavior.
    MappingBehavior behavior;
    // Flags for the mapping.
    MappingFlags flags;
    // Preferred mapping address, or 0 for no preference.
    void* address_hint{nullptr};
    // Size of the mapping in bytes.
    usize size{0};
    // Offset into the mapped file.
    usize offset{0};
    // Require the mapping to use address_hint.
    bool enforce_address_hint{false};
};

struct MemoryProtectionOptions {
    usize offset{0};
    usize size{0};
    MappingProtection protection{MappingProtection::NONE};
};

class MemoryMapping {
public:
    MemoryMapping() = default;

    MemoryMapping(MemoryMapping&& other) noexcept :
        _fd(std::move(other._fd)),
        _filepath(std::move(other._filepath)),
        _base_address(std::exchange(other._base_address, MAP_FAILED)),
        _size(std::exchange(other._size, 0)),
        _offset(std::exchange(other._offset, 0)) {}

    MemoryMapping& operator=(MemoryMapping&& other) noexcept {
        if (this != &other) {
            reset();
            _fd = std::move(other._fd);
            _filepath = std::move(other._filepath);
            _base_address = std::exchange(other._base_address, MAP_FAILED);
            _size = std::exchange(other._size, 0);
            _offset = std::exchange(other._offset, 0);
        }

        return *this;
    }

    static Result<MemoryMapping, MemoryMappingCreationError>
    anonymous(MemoryMappingSettings settings) {
        settings.behavior = MappingBehavior::PRIVATE;
        settings.flags |= MappingFlags::ANONYMOUS;
        return create(settings, OwnedFd{}, NONE);
    }

    static Result<MemoryMapping, MemoryMappingCreationError>
    from_fd(MemoryMappingSettings settings, OwnedFd fd) {
        if (fd.is_invalid()) {
            return Err{MemoryMappingCreationError::broken_file_descriptor(Errno::BAD_FD)};
        }

        return create(settings, std::move(fd), NONE);
    }

    static Result<MemoryMapping, MemoryMappingCreationError>
    from_file(MemoryMappingSettings settings, CString path, AccessMode access_mode) {
        OwnedFd fd = posix::open(path.as_cstr(), access_mode.oflags() | OFlag::SYNC);
        if (fd.is_invalid()) {
            return Err{MemoryMappingCreationError::from_errno()};
        }

        return create(settings, std::move(fd), some(std::move(path)));
    }

    OXIDE_DISABLE_COPY(MemoryMapping);

    ~MemoryMapping() { reset(); }

    Result<Empty, MemoryProtectionError> set_permission(MemoryProtectionOptions options) {
        const usize page_size = SystemInfo::page_size();

        // A protection range must contain at least one byte.
        if (options.size == 0) {
            return Err{MemoryProtectionError::INVALID_SIZE};
        }

        // Protection changes are restricted to whole memory pages.
        if (options.size % page_size != 0 || options.offset % page_size != 0) {
            return Err{MemoryProtectionError::NOT_ALIGNED};
        }

        // The requested region must be fully contained within this mapping.
        if (options.offset >= _size || options.size > _size - options.offset) {
            return Err{MemoryProtectionError::INVALID_RANGE};
        }

        auto* address = static_cast<u8*>(_base_address) + options.offset;

        // Apply the new protection flags to the selected region.
        if (posix::mprotect(address, options.size, options.protection) == -1) {
            return Err{MemoryProtectionError::from_errno()};
        }

        return Ok{Empty{}};
    }

    const OwnedFd& fd() const { return _fd; }

    const Option<CString>& filepath() const { return _filepath; }

    const void* base_address() const { return _base_address; }

    void* base_address_mut() { return _base_address; }

    const u8* as_slice() const { return static_cast<const u8*>(_base_address); }
    u8* as_mut_slice() const { return static_cast<u8*>(_base_address); }

    usize size() const { return _size; }

    usize offset() const { return _offset; }

private:
    OwnedFd _fd;
    Option<CString> _filepath;
    void* _base_address{MAP_FAILED};
    usize _size{0};
    usize _offset{0};

    MemoryMapping(
        OwnedFd file_descriptor,
        Option<CString> file_path,
        void* base_address,
        usize size,
        usize offset
    ) :
        _fd(std::move(file_descriptor)),
        _filepath(std::move(file_path)),
        _base_address(base_address),
        _size(size),
        _offset(offset) {}

    static Result<MemoryMapping, MemoryMappingCreationError>
    create(MemoryMappingSettings settings, OwnedFd file_descriptor, Option<CString> file_path) {
        if (settings.size == 0) {
            return Err{MemoryMappingCreationError::INVALID_SIZE};
        }

        void* address = posix::mmap(
            settings.address_hint,
            settings.size,
            settings.protection,
            settings.flags,
            settings.behavior,
            file_descriptor,
            settings.offset
        );

        if (address == MAP_FAILED) {
            return Err{MemoryMappingCreationError::from_errno()};
        }

        MemoryMapping mapping{
            std::move(file_descriptor),
            std::move(file_path),
            address,
            settings.size,
            settings.offset,
        };

        if (settings.enforce_address_hint && address != settings.address_hint) {
            return Err{MemoryMappingCreationError::FAILED_TO_ENFORCE_ADDRESS_HINT};
        }

        return Ok{std::move(mapping)};
    }

    void reset() {
        const int saved_errno = errno;
        if (_base_address == MAP_FAILED) {
            return;
        }

        if (posix::munmap(_base_address, _size) == -1) {
            OXIDE_PANIC("Unable to unmap a mapped memory region.");
        }

        _base_address = MAP_FAILED;
        _size = 0;
        _offset = 0;
        _fd = OwnedFd{};
        _filepath = NONE;
        errno = saved_errno;
    }
};

} // namespace oxide::posix

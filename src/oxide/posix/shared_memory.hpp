#pragma once

#include <atomic>
#include <cstring>
#include <limits>

#include <oxide/c_string.hpp>
#include <oxide/fmt/format.hpp>
#include <oxide/posix/file.hpp>
#include <oxide/posix/ipc.hpp>
#include <oxide/posix/memory_mapping.hpp>
#include <oxide/posix/pal/errno.hpp>
#include <oxide/posix/pal/fcntl.hpp>
#include <oxide/posix/pal/fd.hpp>
#include <oxide/posix/pal/stat.hpp>
#include <oxide/posix/pal/unistd.hpp>
#include <oxide/posix/pal/mman.hpp>

namespace oxide::posix {

namespace detail {

class SharedMemoryConfigError final {
public:
    enum struct Error : u8 {
        INVALID_ARGUMENT,
        ACCESS,
        RANGE,
    };

    OXIDE_REEXPORT_ERROR(INVALID_ARGUMENT);
    OXIDE_REEXPORT_ERROR(ACCESS);
    OXIDE_REEXPORT_ERROR(RANGE);

    static SharedMemoryConfigError invalid_argument() { return Error::INVALID_ARGUMENT; }
    static SharedMemoryConfigError access() { return Error::ACCESS; }
    static SharedMemoryConfigError range() { return Error::RANGE; }

    Errno to_errno() const {
        switch (_error) { // clang-format off
            case Error::INVALID_ARGUMENT: return Errno::INVALID_ARGUMENT;
            case Error::ACCESS: return Errno::ACCESS;
            case Error::RANGE: return Errno::RANGE;
        } // clang-format on
        OXIDE_UNREACHABLE();
    }

    Error match() const { return _error; }

    bool operator==(const SharedMemoryConfigError& other) const { return _error == other._error; }
    bool operator!=(const SharedMemoryConfigError& other) const { return _error != other._error; }

private:
    Error _error;

    SharedMemoryConfigError(Error error) : _error(error) {}
};

struct SharedMemoryConfig {
    CString name;
    Mode mode{Mode::OWNER_ALL};
    AccessMode access_mode;
    Option<CreationPolicy> creation_policy;
    bool memory_locked{false};
    bool has_ownership{true};
    bool zero_memory{true};
    usize size{0};
    usize mapping_offset{0};
    Option<void*> address_hint;

    Option<SharedMemoryConfigError> is_valid(bool creating) const;
};

// Initial builder state.
//
// Supports options common to opening and creating shared memory. From here
// the builder may either open an existing object or transition into creation.
struct OpenOrCreate {};

// Creation has been selected, but the required shared-memory size has not yet
// been provided. create() is intentionally unavailable in this state.
struct CreateNeedsSize {};

// A size has been provided, so create() is available. The size value itself
// is still validated at runtime.
struct Create {};

} // namespace detail

template <class State = detail::OpenOrCreate>
class SharedMemoryBuilder;

// Maps bytes only; callers coordinate payload initialization and name replacement.
class [[nodiscard]] SharedMemory final {
public:
    SharedMemory(SharedMemory&& other) noexcept :
        _name(std::move(other._name)),
        _has_ownership(other._has_ownership.exchange(false, std::memory_order_relaxed)),
        _mapping(std::move(other._mapping)),
        _memory_lock(std::move(other._memory_lock)),
        _mapping_offset(std::exchange(other._mapping_offset, 0)) {}

    SharedMemory& operator=(SharedMemory&& other) noexcept {
        if (this != &other) {
            reset();
            _name = std::move(other._name);
            _has_ownership.store(
                other._has_ownership.exchange(false, std::memory_order_relaxed),
                std::memory_order_relaxed
            );
            _mapping = std::move(other._mapping);
            _memory_lock = std::move(other._memory_lock);
            _mapping_offset = std::exchange(other._mapping_offset, 0);
        }
        return *this;
    }

    [[nodiscard]] static Option<SharedMemory> open(detail::SharedMemoryConfig config);

    OXIDE_DISABLE_COPY(SharedMemory);

    ~SharedMemory() noexcept { reset(); }

    [[nodiscard]] static i32 unlink(const CStr& name) {
        const auto previous = Errno::last();
        const i32 result = posix::shm_unlink(name);
        if (result == 0 || Errno::is(Errno::NOENT)) {
            Errno::set(previous);
        }
        return result;
    }

    [[nodiscard]] i32 unlink() {
        const auto previous = Errno::last();
        const i32 result = posix::shm_unlink(_name.as_cstr());
        if (result == 0 || Errno::is(Errno::NOENT)) {
            // Do not unlink a replacement object when this handle is destroyed.
            release_ownership();
            Errno::set(previous);
        }
        return result;
    }

    static bool has_persistent_shared_memory() { return posix::HAS_PERSISTENT_SHARED_MEMORY; }

    const CString& name() const { return _name; }

    bool has_ownership() const { return _has_ownership.load(std::memory_order_relaxed); }

    void release_ownership() { _has_ownership.store(false, std::memory_order_relaxed); }

    void acquire_ownership() { _has_ownership.store(true, std::memory_order_relaxed); }

    /// Releases this process's mapping and descriptor without unlinking the
    /// shared-memory object. Any pointers into the mapping become invalid.
    void abandon_in_place() noexcept {
        release_ownership();
        reset();
    }

    const u8* base_address() const { return reinterpret_cast<const u8*>(_mapping.base_address()); }

    const u8* as_slice() const { return _mapping.as_slice(); }
    u8* as_mut_slice() const { return _mapping.as_mut_slice(); }

    usize size() const { return _mapping.size(); }

    usize mapping_offset() const { return _mapping_offset; }

private:
    template <class>
    friend class SharedMemoryBuilder;

    CString _name;
    std::atomic<bool> _has_ownership;
    MemoryMapping _mapping;
    Option<MemoryLock> _memory_lock;
    usize _mapping_offset{0};

    SharedMemory(
        CString name,
        bool has_ownership,
        MemoryMapping mapping,
        Option<MemoryLock> memory_lock,
        usize mapping_offset
    ) :
        _name(std::move(name)),
        _has_ownership(has_ownership),
        _mapping(std::move(mapping)),
        _memory_lock(std::move(memory_lock)),
        _mapping_offset(mapping_offset) {}

    void reset() noexcept {
        const int saved_errno = errno;
        // A lock refers to the mapping's address and must be released first.
        _memory_lock = NONE;
        _mapping = MemoryMapping{};
        if (_has_ownership.exchange(false, std::memory_order_relaxed)) {
            if (posix::shm_unlink(_name.as_cstr()) == -1 && !Errno::last().noent()) {
                OXIDE_ERROR("Unable to remove shared memory '{}'", _name.data());
            }
        }
        _mapping_offset = 0;
        errno = saved_errno;
    }

    static OwnedFd create_new(const detail::SharedMemoryConfig& config) {
        return posix::shm_open(
            config.name.as_cstr(),
            config.access_mode.oflags() | OFlag::CREATE | OFlag::EXCLUSIVE,
            config.mode
        );
    }
};

inline Option<detail::SharedMemoryConfigError>
detail::SharedMemoryConfig::is_valid(bool creating) const {
    if (!posix::is_valid_ipc_name(name.as_cstr())) {
        return detail::SharedMemoryConfigError::invalid_argument();
    }
    // Validate the mapping offset is page-aligned.
    const usize page_size = SystemInfo::page_size();
    if (page_size == 0 || mapping_offset % page_size != 0) {
        return detail::SharedMemoryConfigError::invalid_argument();
    }
    if (mapping_offset > static_cast<usize>(std::numeric_limits<off_t>::max())) {
        return detail::SharedMemoryConfigError::range();
    }
    if (address_hint.is_some() && *address_hint == nullptr) {
        return detail::SharedMemoryConfigError::invalid_argument();
    }
    // mmap requires the backing file descriptor to be open for reading
    if (!access_mode.has_read()) {
        return detail::SharedMemoryConfigError::access();
    }

    if (creating) {
        if (access_mode.flags() != AccessMode::READ_WRITE) {
            return detail::SharedMemoryConfigError::access();
        }
        if (size == 0) {
            return detail::SharedMemoryConfigError::invalid_argument();
        }
        if (size > as<usize>(std::numeric_limits<off_t>::max())) {
            return detail::SharedMemoryConfigError::range();
        }
        if (as<usize>(mapping_offset) >= size) {
            return detail::SharedMemoryConfigError::range();
        }
    }

    return NONE;
}

template <>
class [[nodiscard]] SharedMemoryBuilder<detail::OpenOrCreate> final {
public:
    explicit SharedMemoryBuilder(CString name) { _config.name = std::move(name); }

    [[nodiscard]] SharedMemoryBuilder&& memory_locked(bool value) && {
        _config.memory_locked = value;
        return std::move(*this);
    }

    // Map from this page-aligned byte offset to the end of the object.
    [[nodiscard]] SharedMemoryBuilder&& mapping_offset(std::ptrdiff_t value) && {
        _config.mapping_offset = value;
        return std::move(*this);
    }

    // Require the mapping to use this address; nullptr is invalid when specified.
    [[nodiscard]] SharedMemoryBuilder&& address_hint(void* value) && {
        _config.address_hint = value;
        return std::move(*this);
    }

    [[nodiscard]] Option<SharedMemory> open_existing(AccessMode access_mode) && {
        _config.access_mode = access_mode;
        return std::move(*this).open();
    }

    [[nodiscard]] SharedMemoryBuilder<detail::CreateNeedsSize>
    creation_policy(CreationPolicy policy) &&;

private:
    detail::SharedMemoryConfig _config;

    [[nodiscard]] Option<SharedMemory> open() && { return SharedMemory::open(std::move(_config)); }
};

template <>
class [[nodiscard]] SharedMemoryBuilder<detail::CreateNeedsSize> final {
public:
    [[nodiscard]] SharedMemoryBuilder&& mode(Mode value) && {
        _config.mode = value;
        return std::move(*this);
    }

    [[nodiscard]] SharedMemoryBuilder&& zero_memory(bool value) && {
        _config.zero_memory = value;
        return std::move(*this);
    }

    [[nodiscard]] SharedMemoryBuilder&& has_ownership(bool value) && {
        _config.has_ownership = value;
        return std::move(*this);
    }

    [[nodiscard]] SharedMemoryBuilder<detail::Create> size(std::size_t value) &&;

private:
    template <class>
    friend class SharedMemoryBuilder;

    explicit SharedMemoryBuilder(detail::SharedMemoryConfig config) : _config(std::move(config)) {}

    detail::SharedMemoryConfig _config;
};

inline SharedMemoryBuilder<detail::CreateNeedsSize>
SharedMemoryBuilder<detail::OpenOrCreate>::creation_policy(CreationPolicy policy) && {
    _config.access_mode = AccessMode::read_write();
    _config.creation_policy = policy;
    return SharedMemoryBuilder<detail::CreateNeedsSize>{std::move(_config)};
}

template <>
class [[nodiscard]] SharedMemoryBuilder<detail::Create> final {
public:
    [[nodiscard]] SharedMemoryBuilder&& size(std::size_t value) && {
        // Requested total object size; a nonzero offset maps only its suffix.
        _config.size = value;
        return std::move(*this);
    }

    [[nodiscard]] SharedMemoryBuilder&& mode(Mode value) && {
        _config.mode = value;
        return std::move(*this);
    }

    [[nodiscard]] SharedMemoryBuilder&& zero_memory(bool value) && {
        _config.zero_memory = value;
        return std::move(*this);
    }

    [[nodiscard]] SharedMemoryBuilder&& has_ownership(bool value) && {
        _config.has_ownership = value;
        return std::move(*this);
    }

    [[nodiscard]] Option<SharedMemory> create() && {
        return SharedMemory::open(std::move(_config));
    }

private:
    template <class>
    friend class SharedMemoryBuilder;

    explicit SharedMemoryBuilder(detail::SharedMemoryConfig config) : _config(std::move(config)) {}

    detail::SharedMemoryConfig _config;
};

inline SharedMemoryBuilder<detail::Create>
SharedMemoryBuilder<detail::CreateNeedsSize>::size(std::size_t value) && {
    _config.size = value;
    return SharedMemoryBuilder<detail::Create>{std::move(_config)};
}

SharedMemoryBuilder(CString) -> SharedMemoryBuilder<detail::OpenOrCreate>;

[[nodiscard]] inline SharedMemoryBuilder<detail::OpenOrCreate> shared_memory(CStr name) {
    return SharedMemoryBuilder<detail::OpenOrCreate>{CString::from_cstr(name)};
}

[[nodiscard]] inline SharedMemoryBuilder<detail::OpenOrCreate> shared_memory(CString name) {
    return SharedMemoryBuilder<detail::OpenOrCreate>{std::move(name)};
}

} // namespace oxide::posix

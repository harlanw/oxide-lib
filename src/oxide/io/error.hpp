#pragma once

#include <utility>

#include <oxide/num.hpp>
#include <oxide/option.hpp>
#include <oxide/result.hpp>
#include <oxide/string.hpp>

namespace oxide::posix {
class Errno;
} // namespace oxide::posix

namespace oxide::io {

enum class ErrorKind : u8 {
    NotFound,
    PermissionDenied,
    ConnectionRefused,
    ConnectionReset,
    HostUnreachable,
    NetworkUnreachable,
    ConnectionAborted,
    NotConnected,
    AddrInUse,
    AddrNotAvailable,
    NetworkDown,
    BrokenPipe,
    AlreadyExists,
    WouldBlock,
    NotADirectory,
    IsADirectory,
    DirectoryNotEmpty,
    ReadOnlyFilesystem,
    FilesystemLoop,
    StaleNetworkFileHandle,
    InvalidInput,
    InvalidData,
    TimedOut,
    WriteZero,
    StorageFull,
    NotSeekable,
    QuotaExceeded,
    FileTooLarge,
    ResourceBusy,
    ExecutableFileBusy,
    Deadlock,
    CrossesDevices,
    TooManyLinks,
    InvalidFilename,
    ArgumentListTooLong,
    Interrupted,
    UnexpectedEof,
    Unsupported,
    OutOfMemory,
    InProgress,
    TooManyOpenFiles,
    InputOutputError,
    Other,
    Uncategorized,
};

[[nodiscard]] Str error_kind_description(ErrorKind kind) noexcept;

struct alignas(8) SimpleMessage final {
    ErrorKind kind;
    Str message;
};

// 64-bit packed io::Error, following Rust io/error/repr_bitpacked.rs:
//   00: pointer to static SimpleMessage
//   01: pointer to heap-allocated Custom, with low bit set
//   10: 32-bit signed OS error in bits 32..63
//   11: ErrorKind discriminant in bits 32..63
//
// All live instances have a nonzero representation. No allocation for Os,
// Simple, or SimpleMessage. Custom messages require a heap-owned payload.
// The pointer/integer round trip assumes the ordinary flat pointer model
// provided by 64-bit Linux/macOS, and is not strictly portable ISO C++17.
class Error final {
public:
    explicit Error(ErrorKind kind) noexcept : _bits(pack_kind(kind)) {}

    Error(const Error&) = delete;
    Error& operator=(const Error&) = delete;

    Error(Error&& other) noexcept :
        _bits(std::exchange(other._bits, pack_kind(ErrorKind::Other))) {}

    Error& operator=(Error&& other) noexcept {
        if (this != &other) {
            if (tag() == TAG_CUSTOM) {
                destroy_custom();
            }
            _bits = std::exchange(other._bits, pack_kind(ErrorKind::Other));
        }
        return *this;
    }

    ~Error() noexcept {
        if (tag() == TAG_CUSTOM) {
            destroy_custom();
        }
    }

    // The SimpleMessage MUST have static storage duration.
    [[nodiscard]] static Error from_static_message(const SimpleMessage& message) noexcept;
    static Error from_static_message(const SimpleMessage&&) noexcept = delete;

    [[nodiscard]] static Error last_os_error() noexcept;
    [[nodiscard]] static Error from_raw_os_error(int code) noexcept;
    [[nodiscard]] static Error from_errno(posix::Errno error) noexcept;

    [[nodiscard]] static Error with_message(ErrorKind kind, Str message) noexcept;
    [[nodiscard]] static Error with_message(ErrorKind kind, String message) noexcept;

    [[nodiscard]] static Error other(Str message) noexcept {
        return with_message(ErrorKind::Other, message);
    }
    [[nodiscard]] static Error other(String message) noexcept {
        return with_message(ErrorKind::Other, std::move(message));
    }

    [[nodiscard]] ErrorKind kind() const noexcept;
    [[nodiscard]] Option<int> raw_os_error() const noexcept;
    [[nodiscard]] Option<Str> custom_message() const& noexcept;
    Option<Str> custom_message() const&& = delete;
    [[nodiscard]] bool is_interrupted() const noexcept { return kind() == ErrorKind::Interrupted; }
    [[nodiscard]] String to_string() const noexcept;

private:
    struct Custom;
    struct AdoptBits {};

    static constexpr u64 TAG_MASK = 0b11;
    static constexpr u64 TAG_SIMPLE_MESSAGE = 0b00;
    static constexpr u64 TAG_CUSTOM = 0b01;
    static constexpr u64 TAG_OS = 0b10;
    static constexpr u64 TAG_SIMPLE = 0b11;

    static constexpr NonZero<u64> pack_kind(ErrorKind kind) noexcept {
        return NonZero<u64>::from_unchecked((static_cast<u64>(kind) << 32) | TAG_SIMPLE);
    }
    static constexpr NonZero<u64> pack_os(int code) noexcept {
        return NonZero<u64>::from_unchecked((as<u64>(as<u32>(code)) << 32) | TAG_OS);
    }

    explicit Error(NonZero<u64> bits, AdoptBits) noexcept : _bits(bits) {}

    [[nodiscard]] u64 tag() const noexcept { return _bits.get() & TAG_MASK; }
    [[nodiscard]] const SimpleMessage* simple_message() const noexcept;
    [[nodiscard]] Custom* custom() const noexcept;
    [[nodiscard]] posix::Errno os_error() const noexcept;
    void destroy_custom() noexcept;

    NonZero<u64> _bits;
};

static_assert(sizeof(void*) == 8, "packed io::Error requires 64-bit pointers");
static_assert(sizeof(u64) == 8, "packed io::Error requires 64-bit uintptr_t");
static_assert(sizeof(int) == 4, "packed io::Error requires 32-bit OS codes");
static_assert(alignof(SimpleMessage) >= 4, "SimpleMessage must have two free tag bits");
static_assert(sizeof(Error) == 8, "packed io::Error must be one word");

template <typename T>
using Result = oxide::Result<T, Error>;

} // namespace oxide::io

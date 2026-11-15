#include <cerrno>
#include <charconv>

#include <oxide/io/error.hpp>
#include <oxide/num.hpp>
#include <oxide/panic.hpp>
#include <oxide/posix/pal/errno.hpp>

namespace oxide::io {
namespace {

[[nodiscard]] ErrorKind kind_from_errno(posix::Errno error) noexcept {
    using posix::Errno;

    switch (error.match()) { // clang-format off
        case Errno::NOENT:                 return ErrorKind::NotFound;
        case Errno::ACCESS:
        case Errno::PERM:                  return ErrorKind::PermissionDenied;
        case Errno::CONNECTION_REFUSED:    return ErrorKind::ConnectionRefused;
        case Errno::CONNECTION_RESET:      return ErrorKind::ConnectionReset;
        case Errno::HOST_UNREACHABLE:      return ErrorKind::HostUnreachable;
        case Errno::NETWORK_UNREACHABLE:   return ErrorKind::NetworkUnreachable;
        case Errno::CONNECTION_ABORTED:    return ErrorKind::ConnectionAborted;
        case Errno::NOT_CONNECTED:         return ErrorKind::NotConnected;
        case Errno::ADDRESS_IN_USE:        return ErrorKind::AddrInUse;
        case Errno::ADDRESS_NOT_AVAILABLE: return ErrorKind::AddrNotAvailable;
        case Errno::NETWORK_DOWN:          return ErrorKind::NetworkDown;
        case Errno::PIPE:                  return ErrorKind::BrokenPipe;
        case Errno::EXISTS:                return ErrorKind::AlreadyExists;
        case Errno::TRY_AGAIN:             return ErrorKind::WouldBlock;
        #if EWOULDBLOCK != EAGAIN
            case static_cast<Errno::Error>(EWOULDBLOCK): return ErrorKind::WouldBlock;
        #endif
        case Errno::NOT_DIR:          return ErrorKind::NotADirectory;
        case Errno::IS_DIR:           return ErrorKind::IsADirectory;
        case Errno::NOT_EMPTY:        return ErrorKind::DirectoryNotEmpty;
        case Errno::READ_ONLY_FS:     return ErrorKind::ReadOnlyFilesystem;
        case Errno::TOO_MANY_SYMLINKS: return ErrorKind::FilesystemLoop;
        #ifdef ESTALE
            case Errno::STALE: return ErrorKind::StaleNetworkFileHandle;
        #endif
        case Errno::INVALID_ARGUMENT: return ErrorKind::InvalidInput;
        case Errno::TIMED_OUT:        return ErrorKind::TimedOut;
        case Errno::NO_SPACE:         return ErrorKind::StorageFull;
        case Errno::ILLEGAL_SEEK:     return ErrorKind::NotSeekable;
        #ifdef EDQUOT
            case Errno::QUOTA_EXCEEDED: return ErrorKind::QuotaExceeded;
        #endif
        case Errno::FILE_TOO_LARGE:    return ErrorKind::FileTooLarge;
        case Errno::BUSY:              return ErrorKind::ResourceBusy;
        case Errno::TEXT_BUSY:         return ErrorKind::ExecutableFileBusy;
        case Errno::DEADLOCK:          return ErrorKind::Deadlock;
        case Errno::CROSS_DEVICE:      return ErrorKind::CrossesDevices;
        case Errno::TOO_MANY_LINKS:    return ErrorKind::TooManyLinks;
        case Errno::NAME_TOO_LONG:     return ErrorKind::InvalidFilename;
        case Errno::ARG_LIST_TOO_LONG: return ErrorKind::ArgumentListTooLong;
        case Errno::INTERRUPTED:       return ErrorKind::Interrupted;
        case Errno::NOT_IMPLEMENTED:   return ErrorKind::Unsupported;
        case Errno::NOT_SUPPORTED:     return ErrorKind::Unsupported;
        #if EOPNOTSUPP != ENOTSUP
            case static_cast<Errno::Error>(EOPNOTSUPP): return ErrorKind::Unsupported;
        #endif
        case Errno::NO_MEMORY:          return ErrorKind::OutOfMemory;
        case Errno::IN_PROGRESS:        return ErrorKind::InProgress;
        case Errno::PROCESS_FILE_LIMIT:
        case Errno::SYSTEM_FILE_LIMIT:  return ErrorKind::TooManyOpenFiles;
        case Errno::IO:                 return ErrorKind::InputOutputError;
        default:                       return ErrorKind::Uncategorized;
    } // clang-format on
}

// GNU strerror_r returns char*; POSIX/macOS strerror_r returns int.
[[nodiscard, maybe_unused]] const char* strerror_text(const char* result, char*) noexcept {
    return result;
}
[[nodiscard, maybe_unused]] const char* strerror_text(int result, const char* buffer) noexcept {
    return result == 0 ? buffer : nullptr;
}

} // namespace

struct alignas(8) Error::Custom final {
    ErrorKind kind;
    String message;
};

Str error_kind_description(ErrorKind kind) noexcept {
    switch (kind) { // clang-format off
        case ErrorKind::NotFound: return "entity not found";
        case ErrorKind::PermissionDenied: return "permission denied";
        case ErrorKind::ConnectionRefused: return "connection refused";
        case ErrorKind::ConnectionReset: return "connection reset";
        case ErrorKind::HostUnreachable: return "host unreachable";
        case ErrorKind::NetworkUnreachable: return "network unreachable";
        case ErrorKind::ConnectionAborted: return "connection aborted";
        case ErrorKind::NotConnected: return "not connected";
        case ErrorKind::AddrInUse: return "address in use";
        case ErrorKind::AddrNotAvailable: return "address not available";
        case ErrorKind::NetworkDown: return "network down";
        case ErrorKind::BrokenPipe: return "broken pipe";
        case ErrorKind::AlreadyExists: return "entity already exists";
        case ErrorKind::WouldBlock: return "operation would block";
        case ErrorKind::NotADirectory: return "not a directory";
        case ErrorKind::IsADirectory: return "is a directory";
        case ErrorKind::DirectoryNotEmpty: return "directory not empty";
        case ErrorKind::ReadOnlyFilesystem: return "read-only filesystem or storage medium";
        case ErrorKind::FilesystemLoop: return "filesystem loop or indirection limit (e.g. symlink loop)";
        case ErrorKind::StaleNetworkFileHandle: return "stale network file handle";
        case ErrorKind::InvalidInput: return "invalid input parameter";
        case ErrorKind::InvalidData: return "invalid data";
        case ErrorKind::TimedOut: return "timed out";
        case ErrorKind::WriteZero: return "write zero";
        case ErrorKind::StorageFull: return "no storage space";
        case ErrorKind::NotSeekable: return "seek on unseekable file";
        case ErrorKind::QuotaExceeded: return "quota exceeded";
        case ErrorKind::FileTooLarge: return "file too large";
        case ErrorKind::ResourceBusy: return "resource busy";
        case ErrorKind::ExecutableFileBusy: return "executable file busy";
        case ErrorKind::Deadlock: return "deadlock";
        case ErrorKind::CrossesDevices: return "cross-device link or rename";
        case ErrorKind::TooManyLinks: return "too many links";
        case ErrorKind::InvalidFilename: return "invalid filename";
        case ErrorKind::ArgumentListTooLong: return "argument list too long";
        case ErrorKind::Interrupted: return "operation interrupted";
        case ErrorKind::Unsupported: return "unsupported";
        case ErrorKind::UnexpectedEof: return "unexpected end of file";
        case ErrorKind::OutOfMemory: return "out of memory";
        case ErrorKind::InProgress: return "in progress";
        case ErrorKind::TooManyOpenFiles: return "too many open files";
        case ErrorKind::InputOutputError: return "input/output error";
        case ErrorKind::Other: return "other error";
        case ErrorKind::Uncategorized: return "uncategorized error";
    } // clang-format on
    return "unknown error kind";
}

Error Error::from_static_message(const SimpleMessage& message) noexcept {
    // Caller guarantees static lifetime.
    const auto bits = reinterpret_cast<uptr>(&message);
    if (bits == 0 || (bits & TAG_MASK) != TAG_SIMPLE_MESSAGE) {
        std::abort();
    }
    return Error(NonZero<u64>::from_unchecked(bits), AdoptBits{});
}

Error Error::last_os_error() noexcept {
    return from_errno(posix::Errno{errno});
}

Error Error::from_raw_os_error(int code) noexcept {
    return from_errno(posix::Errno{code});
}

Error Error::from_errno(posix::Errno error) noexcept {
    return Error(pack_os(error.code()), AdoptBits{});
}

Error Error::with_message(ErrorKind kind, Str message) noexcept {
    return with_message(kind, String::from_str(message));
}

Error Error::with_message(ErrorKind kind, String message) noexcept {
    // Rust's Box<Custom> also allocates unconditionally for custom errors.
    static_assert(alignof(Custom) >= 4, "Custom requires two free tag bits");
    auto* payload = new (std::nothrow) Custom{kind, std::move(message)};
    if (payload == nullptr) {
        std::abort();
    }
    const auto bits = reinterpret_cast<uptr>(payload);
    if (bits == 0 || (bits & TAG_MASK) != 0) {
        std::abort();
    }
    return Error(NonZero<u64>::from_unchecked(bits | TAG_CUSTOM), AdoptBits{});
}

const SimpleMessage* Error::simple_message() const noexcept {
    // Intentional pointer recovery from the packed integer representation.
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    return reinterpret_cast<const SimpleMessage*>(_bits.get());
}

Error::Custom* Error::custom() const noexcept {
    // Intentional pointer recovery after removing the packed tag bits.
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    return reinterpret_cast<Custom*>(_bits.get() & ~TAG_MASK);
}

posix::Errno Error::os_error() const noexcept {
    const u32 u = static_cast<u32>(_bits.get() >> 32);
    const i64 signed_code = u <= 0x7fffffffU ? as<i64>(u) : as<i64>(u) - (i64{1} << 32);
    return posix::Errno{static_cast<i32>(signed_code)};
}

void Error::destroy_custom() noexcept {
    delete custom();
}

ErrorKind Error::kind() const noexcept {
    switch (tag()) { // clang-format off
        case TAG_SIMPLE_MESSAGE: return simple_message()->kind;
        case TAG_CUSTOM: return custom()->kind;
        case TAG_OS: return kind_from_errno(os_error());
        case TAG_SIMPLE: return static_cast<ErrorKind>(as<u32>(_bits.get() >> 32));
        default: OXIDE_UNREACHABLE();
    } // clang-format on
}

Option<int> Error::raw_os_error() const noexcept {
    if (tag() == TAG_OS) {
        return some(os_error().code());
    }
    return NONE;
}

Option<Str> Error::custom_message() const& noexcept {
    if (tag() == TAG_CUSTOM) {
        return some(custom()->message.as_str());
    }
    return NONE;
}

String Error::to_string() const noexcept {
    switch (tag()) { // clang-format off
        case TAG_SIMPLE_MESSAGE: return String::from_str(simple_message()->message);
        case TAG_CUSTOM: return custom()->message.clone();
        case TAG_SIMPLE: return String::from_str(error_kind_description(kind()));
        case TAG_OS: break;
        default: OXIDE_UNREACHABLE();
    } // clang-format on

    const auto error = os_error();
    const int code = error.code();
    char buffer[256];
    const char* text = strerror_text(::strerror_r(code, buffer, sizeof(buffer)), buffer);
    String result = String::from_str(
        text != nullptr && text[0] != '\0'
            ? Str::from_ref(*text, std::strlen(text))
            : error_kind_description(kind_from_errno(error))
    );

    char digits[32];
    const auto converted = std::to_chars(digits, digits + sizeof(digits), code);
    if (converted.ec == std::errc{}) {
        result.push_str(Str{" (os error "});
        result.push_str(Str::from_ref(digits[0], static_cast<usize>(converted.ptr - digits)));
        result.push_str(Str{")"});
    }
    return result;
}

} // namespace oxide::io

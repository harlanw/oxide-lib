#pragma once

#include <cerrno>

#include <oxide/compiler/macros.hpp>
#include <oxide/bits.hpp>
#include <oxide/types.hpp>

namespace oxide::posix {

class Errno final {
public:
    enum struct Error : i32 {
        UNKNOWN = -1,
        SUCCESS = 0,

        INTERRUPTED = EINTR,
        IO = EIO,
        NOENT = ENOENT,
        NXIO = ENXIO,
        PERM = EPERM,
        SRCH = ESRCH,
        TRY_AGAIN = EAGAIN,

        ACCESS = EACCES,
        ADDRESS_FAMILY_NOT_SUPPORTED = EAFNOSUPPORT,
        ADDRESS_IN_USE = EADDRINUSE,
        ADDRESS_NOT_AVAILABLE = EADDRNOTAVAIL,
        ALREADY = EALREADY,
        ARG_LIST_TOO_LONG = E2BIG,
        BAD_FD = EBADF,
        BAD_MESSAGE = EBADMSG,
        BUSY = EBUSY,
        CANCELED = ECANCELED,
        CONNECTION_ABORTED = ECONNABORTED,
        CONNECTION_REFUSED = ECONNREFUSED,
        CONNECTION_RESET = ECONNRESET,
        CROSS_DEVICE = EXDEV,
        DEADLOCK = EDEADLK,
        DESTINATION_ADDRESS_REQUIRED = EDESTADDRREQ,
        DOMAIN_ERROR = EDOM,
        EXISTS = EEXIST,
        FAULT = EFAULT,
        FILE_TOO_LARGE = EFBIG,
        HOST_UNREACHABLE = EHOSTUNREACH,
        IDENTIFIER_REMOVED = EIDRM,
        ILLEGAL_BYTE_SEQUENCE = EILSEQ,
        ILLEGAL_SEEK = ESPIPE,
        IN_PROGRESS = EINPROGRESS,
        INVALID_ARGUMENT = EINVAL,
        IS_CONNECTED = EISCONN,
        IS_DIR = EISDIR,
        MESSAGE_TOO_LARGE = EMSGSIZE,
        NAME_TOO_LONG = ENAMETOOLONG,
        NETWORK_DOWN = ENETDOWN,
        NETWORK_RESET = ENETRESET,
        NETWORK_UNREACHABLE = ENETUNREACH,
        NO_BUFFERS = ENOBUFS,
        NO_CHILDREN = ECHILD,
        NO_DATA = ENODATA,
        NO_LINK = ENOLINK,
        NO_LOCKS = ENOLCK,
        NO_MEMORY = ENOMEM,
        NO_MESSAGE = ENOMSG,
        NO_PROTOCOL_OPTION = ENOPROTOOPT,
        NO_SPACE = ENOSPC,
        NO_STREAM_RESOURCES = ENOSR,
        NO_TTY = ENOTTY,
        NODEV = ENODEV,
        NOEXEC = ENOEXEC,
        NOT_CONNECTED = ENOTCONN,
        NOT_DIR = ENOTDIR,
        NOT_EMPTY = ENOTEMPTY,
        NOT_IMPLEMENTED = ENOSYS,
        NOT_RECOVERABLE = ENOTRECOVERABLE,
        NOT_SOCKET = ENOTSOCK,
        NOT_STREAM = ENOSTR,
        NOT_SUPPORTED = ENOTSUP,
        OWNER_DEAD = EOWNERDEAD,
        PIPE = EPIPE,
        PROCESS_FILE_LIMIT = EMFILE,
        PROTOCOL = EPROTO,
        PROTOCOL_NOT_SUPPORTED = EPROTONOSUPPORT,
        QUOTA_EXCEEDED = EDQUOT,
        RANGE = ERANGE,
        READ_ONLY_FS = EROFS,
        STALE = ESTALE,
        STREAM_TIMEOUT = ETIME,
        SYSTEM_FILE_LIMIT = ENFILE,
        TEXT_BUSY = ETXTBSY,
        TIMED_OUT = ETIMEDOUT,
        TOO_MANY_LINKS = EMLINK,
        TOO_MANY_SYMLINKS = ELOOP,
        WOULD_OVERFLOW = EOVERFLOW,
        WRONG_PROTOCOL_TYPE = EPROTOTYPE,

        // clang-format off
        #ifdef EMULTIHOP
            MULTIHOP = EMULTIHOP,
        #endif

        #ifdef ENOTBLK
            NOT_BLOCK_DEVICE = ENOTBLK,
        #endif
        // clang-format on
    };

    OXIDE_REEXPORT_ERROR(UNKNOWN);
    OXIDE_REEXPORT_ERROR(SUCCESS);

    OXIDE_REEXPORT_ERROR(INTERRUPTED);
    OXIDE_REEXPORT_ERROR(IO);
    OXIDE_REEXPORT_ERROR(NOENT);
    OXIDE_REEXPORT_ERROR(NXIO);
    OXIDE_REEXPORT_ERROR(PERM);
    OXIDE_REEXPORT_ERROR(SRCH);
    OXIDE_REEXPORT_ERROR(TRY_AGAIN);

    OXIDE_REEXPORT_ERROR(ACCESS);
    OXIDE_REEXPORT_ERROR(ADDRESS_FAMILY_NOT_SUPPORTED);
    OXIDE_REEXPORT_ERROR(ADDRESS_IN_USE);
    OXIDE_REEXPORT_ERROR(ADDRESS_NOT_AVAILABLE);
    OXIDE_REEXPORT_ERROR(ALREADY);
    OXIDE_REEXPORT_ERROR(ARG_LIST_TOO_LONG);
    OXIDE_REEXPORT_ERROR(BAD_FD);
    OXIDE_REEXPORT_ERROR(BAD_MESSAGE);
    OXIDE_REEXPORT_ERROR(BUSY);
    OXIDE_REEXPORT_ERROR(CANCELED);
    OXIDE_REEXPORT_ERROR(CONNECTION_ABORTED);
    OXIDE_REEXPORT_ERROR(CONNECTION_REFUSED);
    OXIDE_REEXPORT_ERROR(CONNECTION_RESET);
    OXIDE_REEXPORT_ERROR(CROSS_DEVICE);
    OXIDE_REEXPORT_ERROR(DEADLOCK);
    OXIDE_REEXPORT_ERROR(DESTINATION_ADDRESS_REQUIRED);
    OXIDE_REEXPORT_ERROR(DOMAIN_ERROR);
    OXIDE_REEXPORT_ERROR(EXISTS);
    OXIDE_REEXPORT_ERROR(FAULT);
    OXIDE_REEXPORT_ERROR(FILE_TOO_LARGE);
    OXIDE_REEXPORT_ERROR(HOST_UNREACHABLE);
    OXIDE_REEXPORT_ERROR(IDENTIFIER_REMOVED);
    OXIDE_REEXPORT_ERROR(ILLEGAL_BYTE_SEQUENCE);
    OXIDE_REEXPORT_ERROR(ILLEGAL_SEEK);
    OXIDE_REEXPORT_ERROR(IN_PROGRESS);
    OXIDE_REEXPORT_ERROR(INVALID_ARGUMENT);
    OXIDE_REEXPORT_ERROR(IS_CONNECTED);
    OXIDE_REEXPORT_ERROR(IS_DIR);
    OXIDE_REEXPORT_ERROR(MESSAGE_TOO_LARGE);
    OXIDE_REEXPORT_ERROR(NAME_TOO_LONG);
    OXIDE_REEXPORT_ERROR(NETWORK_DOWN);
    OXIDE_REEXPORT_ERROR(NETWORK_RESET);
    OXIDE_REEXPORT_ERROR(NETWORK_UNREACHABLE);
    OXIDE_REEXPORT_ERROR(NO_BUFFERS);
    OXIDE_REEXPORT_ERROR(NO_CHILDREN);
    OXIDE_REEXPORT_ERROR(NO_DATA);
    OXIDE_REEXPORT_ERROR(NO_LINK);
    OXIDE_REEXPORT_ERROR(NO_LOCKS);
    OXIDE_REEXPORT_ERROR(NO_MEMORY);
    OXIDE_REEXPORT_ERROR(NO_MESSAGE);
    OXIDE_REEXPORT_ERROR(NO_PROTOCOL_OPTION);
    OXIDE_REEXPORT_ERROR(NO_SPACE);
    OXIDE_REEXPORT_ERROR(NO_STREAM_RESOURCES);
    OXIDE_REEXPORT_ERROR(NO_TTY);
    OXIDE_REEXPORT_ERROR(NODEV);
    OXIDE_REEXPORT_ERROR(NOEXEC);
    OXIDE_REEXPORT_ERROR(NOT_CONNECTED);
    OXIDE_REEXPORT_ERROR(NOT_DIR);
    OXIDE_REEXPORT_ERROR(NOT_EMPTY);
    OXIDE_REEXPORT_ERROR(NOT_IMPLEMENTED);
    OXIDE_REEXPORT_ERROR(NOT_RECOVERABLE);
    OXIDE_REEXPORT_ERROR(NOT_SOCKET);
    OXIDE_REEXPORT_ERROR(NOT_STREAM);
    OXIDE_REEXPORT_ERROR(NOT_SUPPORTED);
    OXIDE_REEXPORT_ERROR(WOULD_OVERFLOW);
    OXIDE_REEXPORT_ERROR(OWNER_DEAD);
    OXIDE_REEXPORT_ERROR(PIPE);
    OXIDE_REEXPORT_ERROR(PROCESS_FILE_LIMIT);
    OXIDE_REEXPORT_ERROR(PROTOCOL_NOT_SUPPORTED);
    OXIDE_REEXPORT_ERROR(PROTOCOL);
    OXIDE_REEXPORT_ERROR(QUOTA_EXCEEDED);
    OXIDE_REEXPORT_ERROR(RANGE);
    OXIDE_REEXPORT_ERROR(READ_ONLY_FS);
    OXIDE_REEXPORT_ERROR(STALE);
    OXIDE_REEXPORT_ERROR(STREAM_TIMEOUT);
    OXIDE_REEXPORT_ERROR(SYSTEM_FILE_LIMIT);
    OXIDE_REEXPORT_ERROR(TEXT_BUSY);
    OXIDE_REEXPORT_ERROR(TIMED_OUT);
    OXIDE_REEXPORT_ERROR(TOO_MANY_LINKS);
    OXIDE_REEXPORT_ERROR(TOO_MANY_SYMLINKS);
    OXIDE_REEXPORT_ERROR(WRONG_PROTOCOL_TYPE);

    // clang-format off
    #ifdef EMULTIHOP
        OXIDE_REEXPORT_ERROR(MULTIHOP);
    #endif

    #ifdef ENOTBLK
        OXIDE_REEXPORT_ERROR(NOT_BLOCK_DEVICE);
    #endif
    // clang-format on

    constexpr Errno(Error error) noexcept : _error(error) {}
    constexpr Errno(i32 code) noexcept : _error(static_cast<Error>(code)) {}

    [[nodiscard]] static Errno last() noexcept {
        switch (errno) { // clang-format off
            case 0: return SUCCESS;

            case EAGAIN: return TRY_AGAIN;
            case EINTR: return INTERRUPTED;
            case EIO: return IO;
            case ENOENT: return NOENT;
            case ENXIO: return NXIO;
            case EPERM: return PERM;
            case ESRCH: return SRCH;

            case E2BIG: return ARG_LIST_TOO_LONG;
            case EACCES: return ACCESS;
            case EADDRINUSE: return ADDRESS_IN_USE;
            case EADDRNOTAVAIL: return ADDRESS_NOT_AVAILABLE;
            case EAFNOSUPPORT: return ADDRESS_FAMILY_NOT_SUPPORTED;
            case EALREADY: return ALREADY;
            case EBADF: return BAD_FD;
            case EBADMSG: return BAD_MESSAGE;
            case EBUSY: return BUSY;
            case ECANCELED: return CANCELED;
            case ECHILD: return NO_CHILDREN;
            case ECONNABORTED: return CONNECTION_ABORTED;
            case ECONNREFUSED: return CONNECTION_REFUSED;
            case ECONNRESET: return CONNECTION_RESET;
            case EDEADLK: return DEADLOCK;
            case EDESTADDRREQ: return DESTINATION_ADDRESS_REQUIRED;
            case EDOM: return DOMAIN_ERROR;
            case EDQUOT: return QUOTA_EXCEEDED;
            case EEXIST: return EXISTS;
            case EFAULT: return FAULT;
            case EFBIG: return FILE_TOO_LARGE;
            case EHOSTUNREACH: return HOST_UNREACHABLE;
            case EIDRM: return IDENTIFIER_REMOVED;
            case EILSEQ: return ILLEGAL_BYTE_SEQUENCE;
            case EINPROGRESS: return IN_PROGRESS;
            case EINVAL: return INVALID_ARGUMENT;
            case EISCONN: return IS_CONNECTED;
            case EISDIR: return IS_DIR;
            case ELOOP: return TOO_MANY_SYMLINKS;
            case EMFILE: return PROCESS_FILE_LIMIT;
            case EMLINK: return TOO_MANY_LINKS;
            case EMSGSIZE: return MESSAGE_TOO_LARGE;
            case ENAMETOOLONG: return NAME_TOO_LONG;
            case ENETDOWN: return NETWORK_DOWN;
            case ENETRESET: return NETWORK_RESET;
            case ENETUNREACH: return NETWORK_UNREACHABLE;
            case ENFILE: return SYSTEM_FILE_LIMIT;
            case ENOBUFS: return NO_BUFFERS;
            case ENODATA: return NO_DATA;
            case ENODEV: return NODEV;
            case ENOEXEC: return NOEXEC;
            case ENOLCK: return NO_LOCKS;
            case ENOLINK: return NO_LINK;
            case ENOMEM: return NO_MEMORY;
            case ENOMSG: return NO_MESSAGE;
            case ENOPROTOOPT: return NO_PROTOCOL_OPTION;
            case ENOSPC: return NO_SPACE;
            case ENOSR: return NO_STREAM_RESOURCES;
            case ENOSTR: return NOT_STREAM;
            case ENOSYS: return NOT_IMPLEMENTED;
            case ENOTCONN: return NOT_CONNECTED;
            case ENOTDIR: return NOT_DIR;
            case ENOTEMPTY: return NOT_EMPTY;
            case ENOTRECOVERABLE: return NOT_RECOVERABLE;
            case ENOTSOCK: return NOT_SOCKET;
            case ENOTSUP: return NOT_SUPPORTED;
            case ENOTTY: return NO_TTY;
            case EOVERFLOW: return WOULD_OVERFLOW;
            case EOWNERDEAD: return OWNER_DEAD;
            case EPIPE: return PIPE;
            case EPROTO: return PROTOCOL;
            case EPROTONOSUPPORT: return PROTOCOL_NOT_SUPPORTED;
            case EPROTOTYPE: return WRONG_PROTOCOL_TYPE;
            case ERANGE: return RANGE;
            case EROFS: return READ_ONLY_FS;
            case ESPIPE: return ILLEGAL_SEEK;
            case ESTALE: return STALE;
            case ETIME: return STREAM_TIMEOUT;
            case ETIMEDOUT: return TIMED_OUT;
            case ETXTBSY: return TEXT_BUSY;
            case EXDEV: return CROSS_DEVICE;

            #if EWOULDBLOCK != EAGAIN
                case EWOULDBLOCK: return TRY_AGAIN;
            #endif

            #if EOPNOTSUPP != ENOTSUP
                case EOPNOTSUPP: return NOT_SUPPORTED;
            #endif

            #ifdef EMULTIHOP
                case EMULTIHOP: return MULTIHOP;
            #endif

            #ifdef ENOTBLK
                case ENOTBLK: return NOT_BLOCK_DEVICE;
            #endif

            default:
                return UNKNOWN;
        } // clang-format on
    }

    static bool is(Error error) noexcept { return errno == static_cast<i32>(error); }
    static bool is_not(Error error) noexcept { return errno != static_cast<i32>(error); }

    // clang-format off
    // TODO: Remove these
    bool again() const noexcept { return _error == TRY_AGAIN; }
    bool exists() const noexcept { return _error == EXISTS; }
    bool in_progress() const noexcept { return _error == IN_PROGRESS; }
    bool interrupted() const noexcept { return _error ==  INTERRUPTED; }
    bool noent() const noexcept { return _error == NOENT; }
    bool timed_out() const noexcept { return _error == TIMED_OUT; }
    // clang-format on

    static void set(Errno error) noexcept { errno = error.code(); }

    static void clear() noexcept { errno = static_cast<i32>(SUCCESS); }

    Error match() noexcept { return _error; }

    [[nodiscard]] constexpr i32 code() const noexcept { return static_cast<i32>(_error); }

    constexpr bool operator==(const Errno& other) const { return _error == other._error; }
    constexpr bool operator!=(const Errno& other) const { return _error != other._error; }
    constexpr bool operator==(const Error error) const { return _error == error; }
    constexpr bool operator!=(const Error error) const { return _error != error; }

private:
    Error _error{UNKNOWN};
};

template <class T>
inline T fail(Errno::Error error) noexcept {
    Errno::set(error);
    return T{};
}

#define WITH_ERRNO(error, value) (::oxide::posix::Errno::set(error), (value))

} // namespace oxide::posix

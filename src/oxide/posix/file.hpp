#pragma once

#include <sys/stat.h>

#include <oxide/macros.hpp>
#include <oxide/types.hpp>
#include <oxide/posix/pal/fcntl.hpp>
#include <oxide/posix/pal/stat.hpp>
#include <oxide/posix/pal/unistd.hpp>

namespace oxide::posix {

struct Time final {
    i64 seconds{0};
    i64 nanoseconds{0};

    static Time from_timespec(const struct timespec& ts) {
        return Time{as<i64>(ts.tv_sec), as<i64>(ts.tv_nsec)};
    }
};

OXIDE_STRONG_NUM(Uid, u32);
OXIDE_STRONG_NUM(Gid, u32);

struct FileType final {
public:
    enum struct Type : u32 {
        FIFO = S_IFIFO,
        CHARACTER = S_IFCHR,
        DIRECTORY = S_IFDIR,
        BLOCK = S_IFBLK,
        FILE = S_IFREG,
        SYMBOLIC = S_IFLNK,
        SOCKET = S_IFSOCK,
        UNKNOWN = 0xFFFF'FFFF,
    };

    FileType() noexcept = default;

    static FileType from_mode(mode_t mode) {
        switch (mode & S_IFMT) { // clang-format off
            case S_IFIFO: return FileType{Type::FIFO};
            case S_IFCHR: return FileType{Type::CHARACTER};
            case S_IFDIR: return FileType{Type::DIRECTORY};
            case S_IFBLK: return FileType{Type::BLOCK};
            case S_IFREG: return FileType{Type::FILE};
            case S_IFLNK: return FileType{Type::SYMBOLIC};
            case S_IFSOCK: return FileType{Type::SOCKET};
            default: return FileType{Type::UNKNOWN};
        } // clang-format on
    }

    i32 get() const { return static_cast<i32>(_type); }

private:
    Type _type{Type::UNKNOWN};

    FileType(Type type) : _type(type) {}
};

struct Metadata final {
    FileType file_type;
    Uid uid{0};
    Gid gid{0};
    u64 size{0};
    u64 block_size{0};
    u64 block_count{0};
    Mode mode;
    Time access_time{};
    Time modification_time{};
    Time creation_time{};

    static Option<Metadata> from_fd(RawFd fd) {
        struct stat st;
        if (posix::fstat(fd, &st) == -1) {
            return NONE;
        }

        Metadata meta;
        meta.file_type = FileType::from_mode(st.st_mode);
        meta.uid = Uid{as<u32>(st.st_uid)};
        meta.gid = Gid{as<u32>(st.st_gid)};
        meta.size = as<u64>(st.st_size);
        meta.block_size = as<u64>(st.st_blksize);
        meta.block_count = as<u64>(st.st_blocks);
        meta.mode = Mode{as<mode_t>(st.st_mode)};

        // clang-format off
        #if OXIDE_TARGET_OS_MACOS
            meta.access_time = Time::from_timespec(st.st_atimespec);
            meta.modification_time = Time::from_timespec(st.st_mtimespec);
            meta.creation_time = Time::from_timespec(st.st_birthtimespec);
        #elif OXIDE_TARGET_OS_LINUX
            meta.access_time = Time::from_timespec(st.st_atim);
            meta.modification_time = Time::from_timespec(st.st_mtim);
            meta.creation_time = Time::from_timespec(st.st_ctim);
        #endif // clang-format on

        return some(meta);
    }
};

} // namespace oxide::posix

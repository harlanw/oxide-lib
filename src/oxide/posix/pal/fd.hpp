#pragma once

#include <atomic>
#include <utility>
#include <unistd.h>

#include <oxide/assert.hpp>
#include <oxide/macros.hpp>
#include <oxide/types.hpp>
#include <oxide/panic.hpp>
#include <oxide/ptr.hpp>
#include <oxide/posix/pal/errno.hpp>

namespace oxide::posix {

struct RawFd final {
    static constexpr i32 INVALID = -1;

    constexpr RawFd() noexcept = default;
    explicit constexpr RawFd(i32 value) noexcept : inner(value) {}

    [[nodiscard]] constexpr bool is_valid() const noexcept { return inner >= 0; }
    [[nodiscard]] constexpr bool is_invalid() const noexcept { return !is_valid(); }

    [[nodiscard]] constexpr operator i32() const noexcept { return inner; }

    i32 inner{INVALID};
};

class OwnedFd final {
public:
    OwnedFd() noexcept = default;

    OwnedFd(OwnedFd&& other) noexcept : _fd(other.release()) {}

    OwnedFd& operator=(OwnedFd&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }

        return *this;
    }

    static OwnedFd invalid() noexcept { return OwnedFd{}; }

    OXIDE_DISABLE_COPY(OwnedFd);

    ~OwnedFd() noexcept { reset(); }

    [[nodiscard]] RawFd release() noexcept { return std::exchange(_fd, RawFd{}); }

    [[nodiscard]] RawFd as_raw_fd() const noexcept { return _fd; }

    [[nodiscard]] RawFd into_raw_fd() noexcept { return release(); }

    [[nodiscard]] bool is_valid() const noexcept { return _fd.is_valid(); }

    [[nodiscard]] bool is_invalid() const noexcept { return !is_valid(); }

    [[nodiscard]] operator RawFd() const noexcept { return as_raw_fd(); }

    struct unsafe final {
        unsafe() = delete;

        [[nodiscard]] static OwnedFd from_raw_fd(RawFd fd) noexcept { return OwnedFd(fd); }
    };

private:
    RawFd _fd;

    explicit OwnedFd(RawFd fd) noexcept : _fd(fd) {}

    void reset(RawFd fd = {}) noexcept {
        // Preserve the caller's errno across internal descriptor cleanup.
        const Errno error = Errno::last();

        if (_fd.is_valid()) {
            hints::discard(::close(_fd.inner));
        }

        _fd = fd;
        Errno::set(error);
    }
};

class SharedFd final {
public:
    explicit SharedFd(OwnedFd&& fd) noexcept :
        _control(new (std::nothrow) ControlBlock(std::move(fd))) {
        if (_control == nullptr) {
            OXIDE_PANIC("SharedFd: out of memory");
        }
    }

    SharedFd(const SharedFd& other) noexcept : _control(other._control) { retain(_control); }

    SharedFd& operator=(const SharedFd& other) noexcept {
        if (this == &other) {
            return *this;
        }

        ControlBlock* next = other._control;

        retain(next);
        release();

        _control = next;

        return *this;
    }

    SharedFd(SharedFd&& other) noexcept : _control(std::exchange(other._control, nullptr)) {}

    SharedFd& operator=(SharedFd&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        release();

        _control = std::exchange(other._control, nullptr);

        return *this;
    }

    ~SharedFd() noexcept { release(); }

    [[nodiscard]] RawFd as_raw_fd() const noexcept {
        return _control != nullptr ? _control->fd.as_raw_fd() : RawFd{};
    }

    [[nodiscard]] bool is_valid() const noexcept {
        return _control != nullptr && _control->fd.is_valid();
    }

    [[nodiscard]] bool is_invalid() const noexcept { return !is_valid(); }

    [[nodiscard]] operator RawFd() const noexcept { return as_raw_fd(); }

    struct unsafe final {
        unsafe() = delete;

        [[nodiscard]] static SharedFd from_raw_fd(RawFd fd) noexcept {
            return SharedFd{
                OwnedFd::unsafe::from_raw_fd(fd),
            };
        }
    };

private:
    struct ControlBlock final {
        explicit ControlBlock(OwnedFd&& fd) noexcept : fd(std::move(fd)) {}

        std::atomic<usize> references{1};
        OwnedFd fd;
    };

    ControlBlock* _control{nullptr};

    static void retain(ControlBlock* control) noexcept {
        if (control == nullptr) {
            return;
        }

        const usize old = control->references.fetch_add(1, std::memory_order_relaxed);

        OXIDE_ASSERT(
            old != std::numeric_limits<usize>::max(),
            "BorrowedFd reference count overflow"
        );
    }

    void release() noexcept {
        ControlBlock* control = std::exchange(_control, nullptr);

        if (control == nullptr) {
            return;
        }

        if (control->references.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete control;
        }
    }
};

class Fd final {
public:
    explicit Fd(OwnedFd&& fd) noexcept : _type(Type::OWNED) {
        oxide::ptr::construct_at(&owned, std::move(fd));
    }

    explicit Fd(SharedFd fd) noexcept : _type(Type::SHARED) {
        oxide::ptr::construct_at(&shared, std::move(fd));
    }

    Fd(Fd&& other) noexcept : _type(other._type) {
        switch (_type) { // clang-format off
            case Type::OWNED: oxide::ptr::construct_at(&owned, std::move(other.owned)); break;
            case Type::SHARED: oxide::ptr::construct_at(&shared, std::move(other.shared)); break;
        } // clang-format on
    }

    Fd& operator=(Fd&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        destroy();

        _type = other._type;

        switch (_type) { // clang-format off
            case Type::OWNED: oxide::ptr::construct_at(&owned, std::move(other.owned)); break;
            case Type::SHARED: oxide::ptr::construct_at(&shared, std::move(other.shared)); break;
        } // clang-format on

        return *this;
    }

    OXIDE_DISABLE_COPY(Fd);

    ~Fd() noexcept { destroy(); }

    [[nodiscard]] RawFd as_raw_fd() const noexcept {
        switch (_type) { // clang-format off
            case Type::OWNED: return owned.as_raw_fd();
            case Type::SHARED: return shared.as_raw_fd();
        } // clang-format on

        OXIDE_UNREACHABLE();
    }

    [[nodiscard]] operator RawFd() const noexcept { return as_raw_fd(); }

    [[nodiscard]] SharedFd clone() noexcept {
        if (_type == Type::SHARED) {
            return shared;
        }

        SharedFd next{std::move(owned)};

        oxide::ptr::destroy_at(&owned);

        oxide::ptr::construct_at(&shared, std::move(next));
        _type = Type::SHARED;

        return shared;
    }

    [[nodiscard]] bool is_owned() const noexcept { return _type == Type::OWNED; }

    [[nodiscard]] bool is_borrowed() const noexcept { return _type == Type::SHARED; }

    [[nodiscard]] bool is_valid() const noexcept { return as_raw_fd().is_valid(); }
    [[nodiscard]] bool is_invalid() const noexcept { return !is_valid(); }

private:
    enum struct Type : u8 {
        OWNED,
        SHARED,
    };

    Type _type;

    union {
        OwnedFd owned;
        SharedFd shared;
    };

    void destroy() noexcept {
        switch (_type) { // clang-format off
            case Type::OWNED: oxide::ptr::destroy_at(&owned); break;
            case Type::SHARED: oxide::ptr::destroy_at(&shared); break;
        } // clang-format on
    }
};

} // namespace oxide::posix

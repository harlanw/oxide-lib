#pragma once

#include <utility>

#include <oxide/log/logger.hpp>
#include <oxide/c_string.hpp>
#include <oxide/posix/pal/semaphore.hpp>

namespace oxide::posix {

class NamedSemaphore final {
public:
    NamedSemaphore(NamedSemaphore&& other) noexcept :
        _name(std::move(other._name)),
        _handle(std::move(other._handle)),
        _unlink_on_drop(std::exchange(other._unlink_on_drop, false)) {}

    NamedSemaphore& operator=(NamedSemaphore&& other) noexcept {
        if (this != &other) {
            drop();

            _name = std::move(other._name);
            _handle = std::move(other._handle);
            _unlink_on_drop = std::exchange(other._unlink_on_drop, false);
        }

        return *this;
    }

    OXIDE_DISABLE_COPY(NamedSemaphore);

    ~NamedSemaphore() { drop(); }

    [[nodiscard]] static Option<NamedSemaphore> open(const CStr& name) {
        if (!is_valid_ipc_name(name)) {
            return WITH_ERRNO(Errno::INVALID_ARGUMENT, NONE);
        }

        auto owned_name = CString::from_cstr(name);
        auto handle = posix::sem_open(owned_name.as_cstr(), CreationPolicy::open_only());

        if (handle.is_none()) {
            return NONE;
        }

        return NamedSemaphore(std::move(owned_name), std::move(handle).unwrap(), false);
    }

    [[nodiscard]] static Option<NamedSemaphore>
    create(const CStr& name, Mode mode, u32 init_value) {
        if (!is_valid_ipc_name(name)) {
            return WITH_ERRNO(Errno::INVALID_ARGUMENT, NONE);
        }

        auto owned_name = CString::from_cstr(name);
        auto handle = posix::
            sem_open(owned_name.as_cstr(), CreationPolicy::create_only(), mode, init_value);

        if (handle.is_none()) {
            return NONE;
        }

        return NamedSemaphore(std::move(owned_name), std::move(handle).unwrap(), true);
    }

    [[nodiscard]] static Option<NamedSemaphore>
    open_or_create(const CStr& name, Mode mode, u32 init_value) {
        if (!is_valid_ipc_name(name)) {
            return WITH_ERRNO(Errno::INVALID_ARGUMENT, NONE);
        }

        auto owned_name = CString::from_cstr(name);

        auto handle = posix::
            sem_open(owned_name.as_cstr(), CreationPolicy::create_only(), mode, init_value);

        if (handle.is_some()) {
            return NamedSemaphore(std::move(owned_name), std::move(handle).unwrap(), true);
        }

        if (Errno::is_not(Errno::EXISTS)) {
            return NONE;
        }

        handle = posix::sem_open(owned_name.as_cstr(), CreationPolicy::open_only());

        if (handle.is_none()) {
            return NONE;
        }

        return NamedSemaphore(std::move(owned_name), std::move(handle).unwrap(), false);
    }

    [[nodiscard]] bool post() const noexcept { return _handle.post(); }

    [[nodiscard]] bool wait() const noexcept { return _handle.wait(); }

    [[nodiscard]] Option<bool> try_wait() const noexcept { return _handle.try_wait(); }

    [[nodiscard]] Option<bool> wait_for(std::chrono::nanoseconds timeout) const noexcept {
        return _handle.wait_for(timeout);
    }

    [[nodiscard]] static i32 unlink(const CStr& name) {
        const auto previous = Errno::last();
        const i32 result = posix::sem_unlink(name);
        if (result == 0 || Errno::is(Errno::NOENT)) {
            Errno::set(previous);
        }
        return result;
    }

    [[nodiscard]] i32 unlink() noexcept {
        const auto previous = Errno::last();
        const i32 result = posix::sem_unlink(_name.as_cstr());
        if (result == 0 || Errno::is(Errno::NOENT)) {
            _unlink_on_drop = false;
            Errno::set(previous);
        }
        return result;
    }

    [[nodiscard]] const CString& name() const noexcept { return _name; }

private:
    CString _name;
    NamedSemHandle _handle;
    bool _unlink_on_drop{false};

    NamedSemaphore(CString name, NamedSemHandle handle, bool unlink_on_drop) noexcept :
        _name(std::move(name)),
        _handle(std::move(handle)),
        _unlink_on_drop(unlink_on_drop) {}

    void drop() noexcept {
        auto handle = std::move(_handle);
        if (handle.is_failed()) {
            return;
        }

        const auto previous = Errno::last();
        if (posix::sem_close(handle) != 0) {
            OXIDE_PANIC("Failed to close named semaphore: {}", Errno::last().code());
        }

        if (_unlink_on_drop && this->unlink() != 0) {
            OXIDE_WARN("Failed to cleanup named semaphore: {}", Errno::last().code());
        }

        Errno::set(previous);
    }
};

} // namespace oxide::posix

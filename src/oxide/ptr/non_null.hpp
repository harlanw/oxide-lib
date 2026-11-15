#pragma once

#include <memory>

#include <oxide/option.hpp>

namespace oxide::ptr {

template <class T>
class NonNull final {
    T* _ptr{nullptr};

public:
    static constexpr Option<NonNull<T>> from_ptr(T* ptr) {
        return ptr == nullptr ? NONE : some(NonNull<T>{ptr});
    }

    static constexpr NonNull<T> from_ref(T& ref) { return NonNull<T>{std::addressof(ref)}; }

    static constexpr NonNull from(T& ref) { return from_ref(ref); }

    constexpr const T* as_ptr() const { return _ptr; }
    constexpr T* as_mut_ptr() const { return _ptr; }

    constexpr const T& as_ref() const { return *_ptr; }
    constexpr T& as_mut_ref() const { return *_ptr; }

    template <class U>
    NonNull<U> cast() const {
        return NonNull<U>::from(*reinterpret_cast<U*>(_ptr));
    }

    constexpr bool operator==(const NonNull& other) const { return _ptr == other._ptr; }
    constexpr bool operator!=(const NonNull& other) const { return !(*this == other); }

private:
    explicit constexpr NonNull(T* ptr) : _ptr(ptr) {}
};

} // namespace oxide::ptr

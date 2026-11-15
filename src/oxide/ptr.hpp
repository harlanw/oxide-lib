#pragma once

#include "oxide/traits/properties.hpp"
#include <memory>
#include <utility>

#include <oxide/traits.hpp>

namespace oxide::ptr {

// Constructs T in caller-provided storage without allocating memory.
//
// The storage must be non-null, writable, properly aligned, and at least sizeof(T) bytes.
// The caller is responsible for object destruction and storage lifetime.
template <typename T, typename... Args>
T* construct_at(T* location, Args&&... args) noexcept(IsNothrowConstructible<T, Args...>{}) {
    // ::new bypasses class-specific allocation functions; void* selects the
    // standard placement overload, which constructs at the supplied address.
    if constexpr (IsArray<T>{}) {
        static_assert(
            sizeof...(Args) == 0,
            "construct_at: array construction does not accept arguments"
        );

        // T already denotes the whole array. T[1] makes the expression return T*
        // (e.g. int (*)[4] for T = int[4]); () value-initializes the elements.
        return ::new (static_cast<void*>(location)) T[1]();
    } else {
        return ::new (static_cast<void*>(location)) T(std::forward<Args>(args)...);
    }
}

// Destroys the live object at a non-null location without releasing its storage.
// Arrays are destroyed in increasing index order. If a destructor throws, the
// exception propagates and later elements are left alive.
template <typename T>
void destroy_at(T* location) noexcept(IsNothrowDestructible<T>{}) {
    if constexpr (IsArray<T>{}) {
        for (auto& value : *location) {
            // Recursion handles nested arrays; addressof bypasses overloaded &.
            oxide::ptr::destroy_at(std::addressof(value));
        }
    } else {
        location->~T();
    }
}

// Value-initializes count contiguous objects in caller-owned storage.
template <typename T>
T* construct_n_at(T* location, usize count) noexcept(IsNothrowDefaultConstructible<T>{}) {
    static_assert(!IsArray<T>{}, "T must not be an array");
    return ::new (static_cast<void*>(location)) T[count]();
}

// Destroys count contiguous objects without deallocating storage.
template <typename T>
void destroy_n_at(T* location, usize count) noexcept(IsNothrowDestructible<T>{}) {
    for (usize i = 0; i < count; ++i) {
        oxide::ptr::destroy_at(location + i);
    }
}

} // namespace oxide::ptr

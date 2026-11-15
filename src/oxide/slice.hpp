#pragma once

#include <oxide/iter.hpp>
#include <oxide/num/types.hpp>
#include <oxide/ptr/non_null.hpp>
#include <oxide/traits.hpp>
#include <oxide/tuple.hpp>

namespace oxide {

// A non-owning view over a contiguous sequence of T.
//
// Slice does not extend the lifetime of its elements. Destroying or relocating
// the underlying storage invalidates the view. Use Slice<const T> for a
// read-only view; const Slice<T> only prevents rebinding the view itself.
template <typename T>
class Slice final {
    static_assert(IsObject<T>{}, "Slice requires an object type");
    static_assert(!IsArray<T>{}, "Slice element type must not be an array");

    ptr::NonNull<T> _data;
    usize _len{0};

    struct IterSource {
        T* current;
        usize remaining;

        [[nodiscard]] constexpr bool empty() const noexcept { return remaining == 0; }

        [[nodiscard]] constexpr T& read() const noexcept { return *current; }

        constexpr void advance() noexcept {
            ++current;
            --remaining;
        }
    };

public:
    // The pointer/reference must designate a valid contiguous range of len elements.
    static constexpr Option<Slice> from_ptr(T* ptr, usize len) noexcept {
        auto data = ptr::NonNull<T>::from_ptr(ptr);
        return data.is_some() ? some(Slice{data.unwrap(), len}) : NONE;
    }

    static constexpr Slice from_ref(T& ref, usize len) noexcept {
        return Slice{ptr::NonNull<T>::from_ref(ref), len};
    }

    [[nodiscard]] constexpr T* data() noexcept { return _data.as_mut_ptr(); }

    [[nodiscard]] constexpr const T* data() const noexcept { return _data.as_ptr(); }

    [[nodiscard]] constexpr usize len() const noexcept { return _len; }

    [[nodiscard]] constexpr T* begin() const noexcept { return _data.as_mut_ptr(); }

    [[nodiscard]] constexpr T* end() const noexcept { return begin() + _len; }

    [[nodiscard]] constexpr auto iter() const noexcept {
        return iter::Iterator<IterSource>(IterSource{begin(), _len});
    }

    // Returns a subslice over the half-open range [start, end).
    // Both indices must be within bounds; an empty range is permitted.
    [[nodiscard]] constexpr Slice slice(usize start, usize end) const noexcept {
        OXIDE_ASSERT(start <= end, "start > end");
        OXIDE_ASSERT(end <= _len, "out of bounds");
        return {ptr::NonNull<T>::from_ptr(begin() + start).unwrap(), end - start};
    }

    // Returns a subslice over [start, len).
    [[nodiscard]] constexpr Slice slice_from(usize start) const noexcept {
        return slice(start, _len);
    }

    // Returns a subslice over [0, end).
    [[nodiscard]] constexpr Slice slice_to(usize end) const noexcept { return slice(0, end); }

    // Splits the slice at mid, returning [0, mid) and [mid, len).
    // mid must be within bounds; either slice may be empty.
    [[nodiscard]] constexpr Pair<Slice, Slice> slice_at(usize mid) const noexcept {
        OXIDE_ASSERT(mid <= _len, "out of bounds");
        return {slice_to(mid), slice_from(mid)};
    }

    [[nodiscard]] constexpr T& last() const noexcept {
        OXIDE_ASSERT(_len > 0, "empty slice");
        return begin()[_len - 1];
    }

    [[nodiscard]] constexpr bool is_empty() const noexcept { return _len == 0; }

    [[nodiscard]] constexpr T& operator[](usize index) const noexcept {
        OXIDE_ASSERT(index < _len, "out of bounds");
        return begin()[index];
    }

    [[nodiscard]] constexpr bool operator==(Slice other) const noexcept {
        if (_len != other._len) {
            return false;
        }

        for (usize i = 0; i < _len; ++i) {
            if (!(begin()[i] == other.begin()[i])) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr bool operator!=(Slice other) const noexcept {
        return !(*this == other);
    }

private:
    constexpr Slice(ptr::NonNull<T> data, usize len) noexcept : _data(data), _len(len) {}
};

} // namespace oxide

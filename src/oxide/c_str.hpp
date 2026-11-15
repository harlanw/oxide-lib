#pragma once

#include <memory>

#include <oxide/num/types.hpp>
#include <oxide/assert.hpp>
#include <oxide/option.hpp>
#include <oxide/str.hpp>

namespace oxide {

// A borrowed NUL-terminated string. Its length and iterator exclude the NUL;
// the backing storage, including the terminator, must outlive the view.
template <typename Char>
class BasicCStr final {
    BasicStr<Char> _s;

public:
    // The slice must contain a NUL; the view stops at the first one.
    static constexpr BasicCStr from_slice(Slice<Char> s) {
        usize len = 0;
        while (len < s.len() && s[len] != '\0') {
            ++len;
        }
        OXIDE_ASSERT(len < s.len(), "CStr requires a NUL terminator");
        return BasicCStr{BasicStr<Char>::from_slice(s.slice_to(len))};
    }

    // A non-null pointer must refer to a valid NUL-terminated string.
    static constexpr Option<BasicCStr> from_ptr(Char* ptr) {
        if (ptr == nullptr) {
            return NONE;
        }

        usize len = 0;
        while (ptr[len] != '\0') {
            ++len;
        }
        return some(BasicCStr{BasicStr<Char>::from_slice(Slice<Char>::from_ref(*ptr, len))});
    }

    static constexpr Option<BasicCStr> from_ref(Char& ref) { return from_ptr(std::addressof(ref)); }

    [[nodiscard]] constexpr Str as_str() const noexcept { return AsStr(_s); }

    [[nodiscard]] constexpr auto iter() const noexcept { return _s.iter(); }

    [[nodiscard]] constexpr bool strip_prefix(BasicCStr prefix) noexcept {
        return _s.strip_prefix(prefix._s);
    }

    [[nodiscard]] constexpr bool starts_with(char prefix) const noexcept {
        return _s.starts_with(prefix);
    }

    [[nodiscard]] constexpr bool starts_with(BasicCStr prefix) const noexcept {
        return _s.starts_with(prefix._s);
    }

    [[nodiscard]] constexpr bool ends_with(char suffix) const noexcept {
        return _s.ends_with(suffix);
    }

    [[nodiscard]] constexpr bool ends_with(BasicCStr suffix) const noexcept {
        return _s.ends_with(suffix._s);
    }

    [[nodiscard]] constexpr bool contains(char value) const noexcept { return _s.contains(value); }

    [[nodiscard]] constexpr bool contains_any(BasicCStr values) const noexcept {
        return _s.contains_any(values._s);
    }

    [[nodiscard]] constexpr bool contains_only(BasicCStr values) const noexcept {
        return _s.contains_only(values._s);
    }

    constexpr Char* data() const noexcept { return _s.data(); }

    constexpr usize len() const noexcept { return _s.len(); }

    [[nodiscard]] constexpr const char& operator[](usize index) const noexcept { return _s[index]; }

    [[nodiscard]] constexpr bool operator==(BasicCStr other) const noexcept {
        return _s == other._s;
    }

    [[nodiscard]] constexpr bool operator!=(BasicCStr other) const noexcept {
        return _s != other._s;
    }

private:
    friend class CString;

    constexpr BasicCStr(BasicStr<Char> s) noexcept : _s(s) {}
};

using CStr = BasicCStr<const char>;

using CStrMut = BasicCStr<char>;

[[nodiscard]] constexpr Str AsStr(CStr value) noexcept {
    return value.as_str();
}

[[nodiscard]] constexpr Str AsStr(CStrMut value) noexcept {
    return value.as_str();
}

[[nodiscard]] constexpr StrMut AsStrMut(CStrMut value) noexcept {
    return StrMut::from_ptr(value.data(), value.len()).unwrap();
}

} // namespace oxide

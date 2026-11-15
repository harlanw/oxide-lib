#pragma once

#include <memory>
#include <utility>

#include <oxide/assert.hpp>
#include <oxide/c_str.hpp>
#include <oxide/option.hpp>
#include <oxide/string.hpp>

namespace oxide {

// An owned C string with no interior NUL bytes. Borrowed views exclude the
// terminator and are invalidated by moves or destruction.
class CString final {
    // Include the trailing NUL in String's length so moves preserve it.
    String _inner;

public:
    CString() noexcept { _inner.push('\0'); }

    CString(const CString&) = delete;
    CString& operator=(const CString&) = delete;

    CString(CString&& other) noexcept : _inner(std::move(other._inner)) {
        // String leaves the source empty; restore its trailing NUL.
        other._inner.push('\0');
    }

    CString& operator=(CString&& other) noexcept {
        if (this != std::addressof(other)) {
            _inner = std::move(other._inner);
            other._inner.push('\0');
        }
        return *this;
    }

    // Reject NUL bytes in the input; CString appends its own terminator.
    [[nodiscard]] static Option<CString> from_str(Str value) noexcept {
        if (value.contains('\0')) {
            return NONE;
        }
        return some(CString{value});
    }

    [[nodiscard]] static CString from_cstr(CStr value) noexcept { return CString{value.as_str()}; }

    [[nodiscard]] CString clone() const noexcept { return CString{as_str()}; }

    [[nodiscard]] usize len() const noexcept { return _inner.len() - 1; }
    [[nodiscard]] bool is_empty() const noexcept { return len() == 0; }

    [[nodiscard]] const char* data() const& noexcept { return _inner.data(); }
    const char* data() const&& = delete;

    [[nodiscard]] Str as_str() const& noexcept { return Str::from_ref(*data(), len()); }
    Str as_str() const&& = delete;

    [[nodiscard]] CStr as_cstr() const& noexcept { return CStr{as_str()}; }
    CStr as_cstr() const&& = delete;

    [[nodiscard]] auto iter() const& noexcept { return as_str().iter(); }
    auto iter() const&& = delete;

    [[nodiscard]] const char& operator[](usize index) const& noexcept {
        OXIDE_ASSERT(index < len(), "out of bounds");
        return data()[index];
    }
    const char& operator[](usize) const&& = delete;

    [[nodiscard]] bool operator==(const CString& other) const noexcept {
        return as_str() == other.as_str();
    }
    [[nodiscard]] bool operator!=(const CString& other) const noexcept { return !(*this == other); }

private:
    // The caller has validated that value contains no NUL bytes.
    explicit CString(Str value) noexcept {
        OXIDE_ASSERT(value.len() < static_cast<usize>(Isize::MAX), "CString capacity overflow");
        _inner.reserve_exact(value.len() + 1);
        _inner.push_str(value);
        _inner.push('\0');
    }
};

[[nodiscard]] inline Str AsStr(const CString& value) noexcept {
    return value.as_str();
}

Str AsStr(CString&&) = delete;
Str AsStr(const CString&&) = delete;

} // namespace oxide

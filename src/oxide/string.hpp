#pragma once

#include <algorithm>
#include <cstring>
#include <memory>
#include <new>
#include <string_view>
#include <utility>

#include <oxide/assert.hpp>
#include <oxide/option.hpp>
#include <oxide/str.hpp>

namespace oxide {

class String final {
    static constexpr usize INLINE_CAPACITY = 24;
    static constexpr usize MAX_CAPACITY = static_cast<usize>(Isize::MAX);

    union Storage {
        char small[INLINE_CAPACITY];
        char* heap;

        constexpr Storage() noexcept : small{} {}
    } _storage;

    usize _len{0};
    usize _capacity{INLINE_CAPACITY};

public:
    String() noexcept = default;

    String(const String&) = delete;
    String& operator=(const String&) = delete;

    String(String&& other) noexcept : String() { *this = std::move(other); }

    String& operator=(String&& other) noexcept {
        if (this == std::addressof(other)) {
            return *this;
        }

        if (is_heap()) {
            delete[] _storage.heap;
        }

        if (other.is_heap()) {
            _storage.heap = other._storage.heap;
            _len = other._len;
            _capacity = other._capacity;

            // Restore a valid, empty inline representation in the source.
            ::new (static_cast<void*>(std::addressof(other._storage))) Storage{};
        } else {
            ::new (static_cast<void*>(std::addressof(_storage))) Storage{};
            if (other._len != 0) {
                std::memcpy(_storage.small, other._storage.small, other._len);
            }
            _len = other._len;
            _capacity = INLINE_CAPACITY;
        }

        other._len = 0;
        other._capacity = INLINE_CAPACITY;
        return *this;
    }

    static String from_str(Str str) noexcept { return String(str); }

    ~String() {
        if (is_heap()) {
            delete[] _storage.heap;
        }
    }

    [[nodiscard]] static String with_capacity(usize capacity) noexcept {
        String result;
        result.reserve_exact(capacity);
        return result;
    }

    [[nodiscard]] String clone() const noexcept { return String(as_str()); }

    [[nodiscard]] usize len() const noexcept { return _len; }
    [[nodiscard]] usize capacity() const noexcept { return _capacity; }
    [[nodiscard]] bool is_empty() const noexcept { return _len == 0; }

    [[nodiscard]] char* data() & noexcept { return is_heap() ? _storage.heap : _storage.small; }

    [[nodiscard]] const char* data() const& noexcept {
        return is_heap() ? _storage.heap : _storage.small;
    }

    char* data() && = delete;
    const char* data() const&& = delete;

    [[nodiscard]] Str as_str() const& noexcept { return Str::from_ref(*data(), _len); }
    Str as_str() const&& = delete;

    [[nodiscard]] StrMut as_mut_str() & noexcept { return StrMut::from_ref(*data(), _len); }

    [[nodiscard]] auto iter() const& noexcept { return as_str().iter(); }
    auto iter() const&& = delete;

    [[nodiscard]] char& operator[](usize index) & noexcept {
        OXIDE_ASSERT(index < _len, "String index out of bounds");
        return data()[index];
    }

    [[nodiscard]] const char& operator[](usize index) const& noexcept {
        OXIDE_ASSERT(index < _len, "String index out of bounds");
        return data()[index];
    }

    char& operator[](usize) && = delete;
    const char& operator[](usize) const&& = delete;

    void reserve(usize additional) noexcept {
        const usize required = required_size(additional);
        if (required > _capacity) {
            reallocate(growth_capacity(required));
        }
    }

    void reserve_exact(usize additional) noexcept {
        const usize required = required_size(additional);
        if (required > _capacity) {
            reallocate(required);
        }
    }

    void push(char value) noexcept {
        reserve(1);
        data()[_len++] = value;
    }

    void push_str(Str value) noexcept {
        if (value.len() == 0) {
            return;
        }

        const usize required = required_size(value.len());
        if (required > _capacity) {
            const usize capacity = growth_capacity(required);
            auto next = allocate(capacity);

            if (_len != 0) {
                std::memcpy(next.get(), data(), _len);
            }
            // Read value before releasing the old allocation.
            std::memcpy(next.get() + _len, value.data(), value.len());
            install_heap(std::move(next), capacity);
        } else {
            std::memmove(data() + _len, value.data(), value.len());
        }
        _len = required;
    }

    void push_str(std::string_view value) noexcept { push_str(AsStr(value)); }

    [[nodiscard]] Option<char> pop() noexcept {
        if (is_empty()) {
            return NONE;
        }
        const char value = data()[--_len];
        return value;
    }

    void truncate(usize len) noexcept { _len = std::min(len, _len); }

    void clear() noexcept { _len = 0; }

    void shrink_to_fit() noexcept {
        if (_len <= INLINE_CAPACITY) {
            if (is_heap()) {
                char* old = _storage.heap;
                ::new (static_cast<void*>(std::addressof(_storage))) Storage{};
                if (_len != 0) {
                    std::memcpy(_storage.small, old, _len);
                }
                delete[] old;
                _capacity = INLINE_CAPACITY;
            }
        } else if (_len < _capacity) {
            reallocate(_len);
        }
    }

    struct unsafe final {
        unsafe() = delete;

        // The caller must have initialized all bytes in [0, len).
        static void set_len(String& value, usize len) noexcept {
            OXIDE_ASSERT(len <= value._capacity, "String length exceeds capacity");
            value._len = len;
        }
    };

    [[nodiscard]] bool operator==(const String& other) const noexcept {
        return as_str() == other.as_str();
    }

    [[nodiscard]] bool operator!=(const String& other) const noexcept { return !(*this == other); }

private:
    explicit String(Str value) noexcept {
        reserve_exact(value.len());
        push_str(value);
    }

    [[nodiscard]] bool is_heap() const noexcept { return _capacity > INLINE_CAPACITY; }

    [[nodiscard]] usize required_size(usize additional) const noexcept {
        OXIDE_ASSERT(additional <= MAX_CAPACITY - _len, "String capacity overflow");
        return _len + additional;
    }

    [[nodiscard]] usize growth_capacity(usize required) const noexcept {
        const usize doubled = _capacity <= MAX_CAPACITY / 2 ? _capacity * 2 : MAX_CAPACITY;
        return std::max(required, doubled);
    }

    [[nodiscard]] static std::unique_ptr<char[]> allocate(usize capacity) noexcept {
        OXIDE_ASSERT(capacity <= MAX_CAPACITY, "String capacity overflow");
        std::unique_ptr<char[]> result(new (std::nothrow) char[capacity]);
        OXIDE_ASSERT(result != nullptr, "String allocation failed");
        return result;
    }

    void install_heap(std::unique_ptr<char[]> next, usize capacity) noexcept {
        char* old = is_heap() ? _storage.heap : nullptr;
        _storage.heap = next.release();
        _capacity = capacity;
        delete[] old;
    }

    void reallocate(usize capacity) noexcept {
        OXIDE_ASSERT(capacity > INLINE_CAPACITY && capacity >= _len, "Invalid String capacity");
        auto next = allocate(capacity);
        if (_len != 0) {
            std::memcpy(next.get(), data(), _len);
        }
        install_heap(std::move(next), capacity);
    }
};

[[nodiscard]] inline Str AsStr(const String& value) noexcept {
    return value.as_str();
}

[[nodiscard]] inline StrMut AsStrMut(String& value) noexcept {
    return value.as_mut_str();
}

Str AsStr(String&&) = delete;
Str AsStr(const String&&) = delete;

} // namespace oxide

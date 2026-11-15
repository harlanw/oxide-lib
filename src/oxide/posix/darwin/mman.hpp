#pragma once

#include <sys/posix_shm.h>

#include <array>
#include <cstring>

#include <CommonCrypto/CommonDigest.h>

#include <oxide/c_string.hpp>

// Darwin shared-memory backend implementation.
namespace oxide::posix::darwin {

namespace detail {

inline constexpr Str SHM_HASH_PREFIX{"ox_"};
inline constexpr Str BASE32_ALPHABET{"0123456789ABCDEFGHJKMNPQRSTVWXYZ"};

inline constexpr usize SHM_HASH_BYTES = 16;
inline constexpr usize SHM_HASH_CHARS = 26;

// '/' + "ox_" + 26 base32 characters.
static_assert(1 + SHM_HASH_PREFIX.len() + SHM_HASH_CHARS <= PSHMNAMLEN);

static_assert(1 + SHM_HASH_PREFIX.len() + SHM_HASH_CHARS <= PSHMNAMLEN);
static_assert(SHM_HASH_CHARS * 5 <= (SHM_HASH_BYTES * 8) + 4);

[[nodiscard]] inline std::array<char, SHM_HASH_CHARS> encode_shm_hash(const u8* bytes) noexcept {
    std::array<char, SHM_HASH_CHARS> encoded{};

    u16 buffer = 0;
    usize bits = 0;
    usize byte = 0;

    for (usize i = 0; i < SHM_HASH_CHARS; ++i) {
        if (bits < 5 && byte < SHM_HASH_BYTES) {
            buffer = static_cast<u16>((buffer << 8) | bytes[byte++]);
            bits += 8;
        }

        if (bits >= 5) {
            bits -= 5;
            encoded[i] = BASE32_ALPHABET[(buffer >> bits) & 0x1F];
        } else {
            // Zero-pad the final partial group.
            encoded[i] = BASE32_ALPHABET[(buffer << (5 - bits)) & 0x1F];
            bits = 0;
        }
    }

    return encoded;
}

} // namespace detail

// Maps an Oxide logical shared-memory name to a macOS-compatible POSIX
// shared-memory name.
//
// Short names are preserved directly. "ox_" is reserved for generated names,
// so logical names beginning with that prefix are hashed as well.
[[nodiscard]] inline Option<CString> shm_native_name(CStr name) noexcept {
    auto logical_name = name.as_str();
    static_cast<void>(logical_name.strip_prefix("/"));

    std::array<char, PSHMNAMLEN> native{};
    native[0] = '/';

    // Preserve ordinary names that already fit in Darwin's native limit.
    if (logical_name.len() < PSHMNAMLEN && !logical_name.starts_with(detail::SHM_HASH_PREFIX)) {
        std::memcpy(native.data() + 1, logical_name.data(), logical_name.len());
        return CString::from_str(Str::from_ref(native[0], logical_name.len() + 1));
    }

    std::array<u8, CC_SHA256_DIGEST_LENGTH> digest{};

    CC_SHA256(logical_name.data(), static_cast<CC_LONG>(logical_name.len()), digest.data());

    const auto encoded = detail::encode_shm_hash(digest.data());

    std::memcpy(native.data() + 1, detail::SHM_HASH_PREFIX.data(), detail::SHM_HASH_PREFIX.len());
    std::memcpy(native.data() + 1 + detail::SHM_HASH_PREFIX.len(), encoded.data(), encoded.size());
    const usize len = 1 + detail::SHM_HASH_PREFIX.len() + encoded.size();
    return CString::from_str(Str::from_ref(native[0], len));
}

} // namespace oxide::posix::darwin

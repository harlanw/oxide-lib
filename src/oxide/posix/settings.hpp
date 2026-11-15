#pragma once

#include <array>
#include <cstring>
#include <string_view>

#include <oxide/assert.hpp>
#include <oxide/env.hpp>
#include <oxide/posix/limits.hpp>

namespace oxide::posix {

inline constexpr const char SHM_LOOKUP_DIR[] = "oxide/shm";
inline constexpr const char SHM_LOOKUP_SUFFIX[] = ".shm_lookup";

[[nodiscard]] inline std::string_view shm_lookup_dir() noexcept {
    static const auto path = [] {
        auto dir_view = std::string_view(SHM_LOOKUP_DIR);

        std::array<char, posix::MAX_PATH_LEN> result{};

        const std::string_view base{env::tmp()};
        const bool needs_sep = !base.empty() && base.back() != '/';
        const usize path_len = base.size() + static_cast<usize>(needs_sep) + dir_view.size();

        // MAX_PATH_LEN includes the null terminator.
        OXIDE_ASSERT(path_len < result.size(), "Temporary directory path is too long");

        usize offset = 0;
        std::memcpy(result.data(), base.data(), base.size());
        offset += base.size();

        if (needs_sep) {
            result[offset] = '/';
            offset++;
        }

        std::memcpy(result.data() + offset, dir_view.data(), dir_view.size());
        result[path_len] = '\0';

        return result;
    }();
    static const std::string_view view{path.data()};

    return view;
}

} // namespace oxide::posix

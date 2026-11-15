#pragma once

#include <oxide/bits.hpp>
#include <oxide/panic.hpp>
#include <oxide/posix/pal/fcntl.hpp>

namespace oxide::posix {

class CreationPolicy {
public:
    enum struct Type : i32 {
        CREATE_ONLY,
        OPEN_ONLY,
        OPEN_OR_CREATE,
    };

    OXIDE_REEXPORT_TYPE(CREATE_ONLY);
    OXIDE_REEXPORT_TYPE(OPEN_ONLY);
    OXIDE_REEXPORT_TYPE(OPEN_OR_CREATE);

    static constexpr CreationPolicy create_only() { return Type::CREATE_ONLY; }
    static constexpr CreationPolicy open_only() { return Type::OPEN_ONLY; }
    static constexpr CreationPolicy open_or_create() { return Type::OPEN_OR_CREATE; }

    constexpr Type match() const { return _type; }

    constexpr OFlag oflags() const {
        switch (_type) { // clang-format off
            case Type::CREATE_ONLY: return OFlag::CREATE | OFlag::EXCLUSIVE;
            case Type::OPEN_ONLY: return OFlag::READ_ONLY;
            case Type::OPEN_OR_CREATE: return OFlag::CREATE | OFlag::READ_ONLY;
        } // clang-format on
        OXIDE_UNREACHABLE();
    }

    constexpr i32 bits() const { return oflags().bits(); }

    constexpr bool operator==(const CreationPolicy& other) const { return _type == other._type; }
    constexpr bool operator!=(const CreationPolicy& other) const { return _type != other._type; }

private:
    Type _type;

    constexpr CreationPolicy(Type type) : _type(type) {}
};

// The purpose of this function is to enforce the POSIX rules for IPC names.
// POSIX only guarantees portable IPC behavior when the name begins with '/'.
[[nodiscard]] inline bool is_valid_ipc_name(CStr name) noexcept {
    auto value = name.as_str();
    static_cast<void>(value.strip_prefix("/"));

    if (value.contains('/') || value.contains('\0')) {
        return false;
    }

    return value.len() <= NAME_MAX;
}

} // namespace oxide::posix

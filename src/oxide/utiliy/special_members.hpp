#pragma once

#include <oxide/traits.hpp>

namespace oxide::special_members {

namespace detail {

template <typename T, bool Copy = IsCopyConstructible<T>{}, bool Move = IsMoveConstructible<T>{}>
struct Ctor {
    Ctor() = default;
    Ctor(const Ctor&) = default;
    Ctor(Ctor&&) noexcept = default;

    Ctor& operator=(const Ctor&) = default;
    Ctor& operator=(Ctor&&) noexcept = default;
};

template <typename T>
struct Ctor<T, false, false> {
    Ctor() = default;
    Ctor(const Ctor&) = delete;
    Ctor(Ctor&&) noexcept = delete;

    Ctor& operator=(const Ctor&) = default;
    Ctor& operator=(Ctor&&) noexcept = default;
};

template <typename T>
struct Ctor<T, false, true> {
    Ctor() = default;
    Ctor(const Ctor&) = delete;
    Ctor(Ctor&&) noexcept = default;

    Ctor& operator=(const Ctor&) = default;
    Ctor& operator=(Ctor&&) noexcept = default;
};

template <typename T>
struct Ctor<T, true, false> {
    Ctor() = default;
    Ctor(const Ctor&) = default;
    Ctor(Ctor&&) noexcept = delete;

    Ctor& operator=(const Ctor&) = default;
    Ctor& operator=(Ctor&&) noexcept = default;
};

template <typename T, bool Copy = IsCopyable<T>{}, bool Move = IsMovable<T>{}>
struct Assign {
    Assign() = default;
    Assign(const Assign&) = default;
    Assign(Assign&&) noexcept = default;

    Assign& operator=(const Assign&) = default;
    Assign& operator=(Assign&&) noexcept = default;
};

template <typename T>
struct Assign<T, false, false> {
    Assign() = default;
    Assign(const Assign&) = default;
    Assign(Assign&&) noexcept = default;

    Assign& operator=(const Assign&) = delete;
    Assign& operator=(Assign&&) noexcept = delete;
};

template <typename T>
struct Assign<T, false, true> {
    Assign() = default;
    Assign(const Assign&) = default;
    Assign(Assign&&) noexcept = default;

    Assign& operator=(const Assign&) = delete;
    Assign& operator=(Assign&&) noexcept = default;
};

template <typename T>
struct Assign<T, true, false> {
    Assign() = default;
    Assign(const Assign&) = default;
    Assign(Assign&&) noexcept = default;

    Assign& operator=(const Assign&) = default;
    Assign& operator=(Assign&&) noexcept = delete;
};

} // namespace detail

template <typename... Ts>
using Ctor = detail::
    Ctor<void, (IsCopyConstructible<Ts>{} && ...), (IsMoveConstructible<Ts>{} && ...)>;

template <typename... Ts>
using Assign = detail::Assign<void, (IsCopyable<Ts>{} && ...), (IsMovable<Ts>{} && ...)>;

} // namespace oxide::special_members

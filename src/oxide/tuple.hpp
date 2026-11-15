#pragma once

#include <utility>

#include <oxide/num/types.hpp>
#include <oxide/traits.hpp>

namespace oxide {

template <class... Ts>
class Tuple;

namespace detail::tuple {

struct Init {};

template <usize I, class... Ts>
struct Element {
    static_assert(I < sizeof...(Ts), "Tuple index is out of range");
};

template <usize I, class T, class... Ts>
struct Element<I, T, Ts...> : Element<I - 1, Ts...> {};

template <class T, class... Ts>
struct Element<0, T, Ts...> {
    using type = T;
};

template <
    class Owner,
    usize I,
    class T,
    bool = IsEmpty<T>{} && !IsFinal<T>{} && IsSame<T, RemoveCv<T>>{}>
class Leaf {
    T _value;

public:
    template <bool Enabled = IsDefaultConstructible<T>{}, EnableIf<Enabled, int> = 0>
    constexpr Leaf() noexcept(IsNothrowDefaultConstructible<T>{}) : _value() {}

    template <class U, EnableIf<IsConstructible<T, U&&>{}, int> = 0>
    constexpr Leaf(Init, U&& value) noexcept(IsNothrowConstructible<T, U&&>{}) :
        _value(std::forward<U>(value)) {}

    constexpr T& value() noexcept { return _value; }
    constexpr const T& value() const noexcept { return _value; }
};

template <class Owner, usize I, class T>
class Leaf<Owner, I, T, true> : private T {
public:
    template <bool Enabled = IsDefaultConstructible<T>{}, EnableIf<Enabled, int> = 0>
    constexpr Leaf() noexcept(IsNothrowDefaultConstructible<T>{}) : T() {}

    template <class U, EnableIf<IsConstructible<T, U&&>{}, int> = 0>
    constexpr Leaf(Init, U&& value) noexcept(IsNothrowConstructible<T, U&&>{}) :
        T(std::forward<U>(value)) {}

    constexpr T& value() noexcept { return *this; }
    constexpr const T& value() const noexcept { return *this; }
};

template <class Owner, usize I, class T>
class Leaf<Owner, I, T&, false> {
    T* _value;

public:
    Leaf() = delete;

    template <class U, EnableIf<IsConvertible<U*, T*>{}, int> = 0>
    constexpr Leaf(Init, U& value) noexcept : _value(std::addressof(value)) {}

    constexpr T& value() const noexcept { return *_value; }
};

template <typename Indices, typename... Ts>
struct Storage;

template <usize... Is, typename... Ts>
struct Storage<std::index_sequence<Is...>, Ts...> : Leaf<Tuple<Ts...>, Is, Ts>... {
    Storage() = default;

    template <
        typename... Us,
        EnableIf<Conjunction<IsConstructible<Leaf<Tuple<Ts...>, Is, Ts>, Init, Us&&>...>{}, int> =
            0>
    constexpr Storage(Init, Us&&... values) noexcept(
        (IsNothrowConstructible<Leaf<Tuple<Ts...>, Is, Ts>, Init, Us&&>{} && ...)
    ) :
        Leaf<Tuple<Ts...>, Is, Ts>(Init{}, std::forward<Us>(values))... {}
};

} // namespace detail::tuple

template <typename... Ts>
class Tuple : private detail::tuple::Storage<std::index_sequence_for<Ts...>, Ts...> {
    static_assert(
        ((IsObject<Ts>{} || IsLvalueRef<Ts>{}) && ...),
        "Tuple elements must be object types or lvalue references"
    );
    static_assert((!IsArray<Ts>{} && ...), "Use a wrapper for owned array elements");

    using Base = detail::tuple::Storage<std::index_sequence_for<Ts...>, Ts...>;

    template <usize I>
    using Leaf = detail::tuple::Leaf<Tuple, I, typename detail::tuple::Element<I, Ts...>::type>;

public:
    template <usize I>
    using Element = typename detail::tuple::Element<I, Ts...>::type;

    // Value-initialize every field, including scalar fields. Disabled when
    // any field has no default constructor (including borrowed fields).
    template <bool Enabled = (IsDefaultConstructible<Ts>{} && ...), EnableIf<Enabled, int> = 0>
    constexpr Tuple() noexcept((IsNothrowDefaultConstructible<Ts>{} && ...)) : Base{} {}

    // Prevent single-argument forwarding from hijacking native copy/move.
    // Separate SFINAE parameters stop substitution before testing the storage
    // constructor when the argument is this exact Tuple type.
    template <
        typename U,
        typename... Us,
        EnableIf<sizeof...(Us) != 0 || !IsSame<Tuple, Decay<U>>{}, int> = 0,
        EnableIf<IsConstructible<Base, detail::tuple::Init, U&&, Us&&...>{}, int> = 0>
    constexpr explicit Tuple(U&& first, Us&&... rest) noexcept(
        IsNothrowConstructible<Base, detail::tuple::Init, U&&, Us&&...>{}
    ) :
        Base(detail::tuple::Init{}, std::forward<U>(first), std::forward<Us>(rest)...) {}

    template <usize I>
    [[nodiscard]] constexpr Element<I>& get() & noexcept {
        return static_cast<Leaf<I>&>(*this).value();
    }

    template <usize I>
    [[nodiscard]] constexpr const Element<I>& get() const& noexcept {
        return static_cast<const Leaf<I>&>(*this).value();
    }

    template <usize I>
    [[nodiscard]] constexpr Element<I>&& get() && noexcept {
        return static_cast<Element<I>&&>(static_cast<Leaf<I>&>(*this).value());
    }

    template <usize I>
    [[nodiscard]] constexpr const Element<I>&& get() const&& noexcept {
        return static_cast<const Element<I>&&>(static_cast<const Leaf<I>&>(*this).value());
    }

    [[nodiscard]] static constexpr usize len() noexcept { return sizeof...(Ts); }
    [[nodiscard]] static constexpr bool is_empty() noexcept { return sizeof...(Ts) == 0; }
};

template <>
class Tuple<> {
public:
    [[nodiscard]] static constexpr usize len() noexcept { return 0; }
    [[nodiscard]] static constexpr bool is_empty() noexcept { return true; }
};

// Automatically determines the Tuple's element types from its arguments.
template <typename... Ts>
Tuple(Ts...) -> Tuple<Ts...>;

using Empty = Tuple<>;

template <typename T, typename U>
using Pair = Tuple<T, U>;

template <typename... Ts>
[[nodiscard]] constexpr Tuple<Decay<Ts>...>
tuple(Ts&&... values) noexcept(IsNothrowConstructible<Tuple<Decay<Ts>...>, Ts&&...>{}) {
    return Tuple<Decay<Ts>...>(std::forward<Ts>(values)...);
}

template <typename T, typename U>
[[nodiscard]] constexpr Pair<Decay<T>, Decay<U>>
pair(T&& first, U&& second) noexcept(IsNothrowConstructible<Pair<Decay<T>, Decay<U>>, T&&, U&&>{}) {
    return Pair<Decay<T>, Decay<U>>(std::forward<T>(first), std::forward<U>(second));
}

template <usize I, typename... Ts>
[[nodiscard]] constexpr decltype(auto) get(Tuple<Ts...>& value) noexcept {
    return value.template get<I>();
}

template <usize I, typename... Ts>
[[nodiscard]] constexpr decltype(auto) get(const Tuple<Ts...>& value) noexcept {
    return value.template get<I>();
}

template <usize I, typename... Ts>
[[nodiscard]] constexpr decltype(auto) get(Tuple<Ts...>&& value) noexcept {
    return std::move(value).template get<I>();
}

template <usize I, typename... Ts>
[[nodiscard]] constexpr decltype(auto) get(const Tuple<Ts...>&& value) noexcept {
    return std::move(value).template get<I>();
}

namespace detail::tuple {

template <typename... Ts, usize... Is>
constexpr bool
equal(const Tuple<Ts...>& a, const Tuple<Ts...>& b, std::index_sequence<Is...>) noexcept(
    (IsEquatable<Ts>::nothrow && ...)
) {
    return (static_cast<bool>(a.template get<Is>() == b.template get<Is>()) && ...);
}

// Stop at the first unequal field. In particular, NaN is not mistaken for
// IsEquatable: unordered fields produce false rather than comparing later fields.
template <bool Inclusive, usize I = 0, typename... Ts>
constexpr bool
less(const Tuple<Ts...>& a, const Tuple<Ts...>& b) noexcept((IsOrderable<Ts>::nothrow && ...)) {
    if constexpr (I == sizeof...(Ts)) {
        return Inclusive;
    } else {
        if (a.template get<I>() == b.template get<I>()) {
            return less<Inclusive, I + 1>(a, b);
        }
        return static_cast<bool>(a.template get<I>() < b.template get<I>());
    }
}

} // namespace detail::tuple

template <typename... Ts, EnableIf<Conjunction<IsEquatable<Ts>...>{}, int> = 0>
[[nodiscard]] constexpr bool operator==(const Tuple<Ts...>& a, const Tuple<Ts...>& b) noexcept(
    (IsEquatable<Ts>::nothrow && ...)
) {
    return detail::tuple::equal(a, b, std::index_sequence_for<Ts...>{});
}

template <typename... Ts, EnableIf<Conjunction<IsEquatable<Ts>...>{}, int> = 0>
[[nodiscard]] constexpr bool
operator!=(const Tuple<Ts...>& a, const Tuple<Ts...>& b) noexcept(noexcept(a == b)) {
    return !(a == b);
}

template <typename... Ts, EnableIf<Conjunction<IsOrderable<Ts>...>{}, int> = 0>
[[nodiscard]] constexpr bool operator<(const Tuple<Ts...>& a, const Tuple<Ts...>& b) noexcept(
    (IsOrderable<Ts>::nothrow && ...)
) {
    return detail::tuple::less<false>(a, b);
}

template <typename... Ts, EnableIf<Conjunction<IsOrderable<Ts>...>{}, int> = 0>
[[nodiscard]] constexpr bool operator<=(const Tuple<Ts...>& a, const Tuple<Ts...>& b) noexcept(
    (IsOrderable<Ts>::nothrow && ...)
) {
    return detail::tuple::less<true>(a, b);
}

template <typename... Ts, EnableIf<Conjunction<IsOrderable<Ts>...>{}, int> = 0>
[[nodiscard]] constexpr bool
operator>(const Tuple<Ts...>& a, const Tuple<Ts...>& b) noexcept(noexcept(b < a)) {
    return b < a;
}

template <typename... Ts, EnableIf<Conjunction<IsOrderable<Ts>...>{}, int> = 0>
[[nodiscard]] constexpr bool
operator>=(const Tuple<Ts...>& a, const Tuple<Ts...>& b) noexcept(noexcept(b <= a)) {
    return b <= a;
}

} // namespace oxide

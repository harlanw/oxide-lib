#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include <oxide/assert.hpp>
#include <oxide/option.hpp>
#include <oxide/mem/scope.hpp>
#include <oxide/traits/properties.hpp>
#include <oxide/utiliy/special_members.hpp>
#include <oxide/ptr.hpp>

namespace oxide {

template <typename T, typename E>
class Result;

template <typename T>
struct Ok {
    using value_type = T;
    T value;
};

template <typename T>
Ok(T) -> Ok<T>;

template <typename E>
struct Err {
    using error_type = E;
    E error;
};

template <typename E>
Err(E) -> Err<E>;

namespace detail::result {

struct OkTag {
    explicit constexpr OkTag() = default;
};

struct ErrTag {
    explicit constexpr ErrTag() = default;
};

} // namespace detail::result

template <typename T>
struct IsResult : std::false_type {};

template <typename T, typename E>
struct IsResult<Result<T, E>> : std::true_type {};

namespace detail::result::storage {

enum struct State : u8 {
    Empty,
    Ok,
    Err,
};

template <typename T, typename E, bool = IsTriviallyDestructible<T, E>{}>
struct StorageDtor;

template <typename T, typename E>
struct StorageDtor<T, E, true> {
    union {
        Empty empty;
        T value;
        E error;
    };

    State state;

    constexpr StorageDtor() noexcept : empty{}, state(State::Empty) {}

    template <typename... Args>
    constexpr StorageDtor(result::OkTag, Args&&... args) noexcept(
        IsNothrowConstructible<T, Args...>{}
    ) :
        value(std::forward<Args>(args)...),
        state(State::Ok) {}

    template <typename... Args>
    constexpr StorageDtor(result::ErrTag, Args&&... args) noexcept(
        IsNothrowConstructible<E, Args...>{}
    ) :
        error(std::forward<Args>(args)...),
        state(State::Err) {}
};

template <typename T, typename E>
struct StorageDtor<T, E, false> {
    union {
        Empty empty;
        T value;
        E error;
    };

    State state;

    constexpr StorageDtor() noexcept : empty{}, state(State::Empty) {}

    template <typename... Args>
    constexpr StorageDtor(result::OkTag, Args&&... args) noexcept(
        IsNothrowConstructible<T, Args...>{}
    ) :
        value(std::forward<Args>(args)...),
        state(State::Ok) {}

    template <typename... Args>
    constexpr StorageDtor(result::ErrTag, Args&&... args) noexcept(
        IsNothrowConstructible<E, Args...>{}
    ) :
        error(std::forward<Args>(args)...),
        state(State::Err) {}

    ~StorageDtor() noexcept {
        if (state == State::Ok) {
            value.~T();
        } else if (state == State::Err) {
            error.~E();
        }
    }
};

template <typename T, typename E>
struct Storage : StorageDtor<T, E> {
    using StorageDtor<T, E>::StorageDtor;

    template <typename Other>
    void construct_from(Other&& other) {
        if (other.is_ok()) {
            ptr::construct_at(std::addressof(this->value), std::forward<Other>(other).get());
            this->state = State::Ok;
        } else {
            ptr::construct_at(std::addressof(this->error), std::forward<Other>(other).get_error());
            this->state = State::Err;
        }
    }

    template <typename Other>
    void assign(Other&& other) {
        if (this == std::addressof(other)) {
            return;
        }

        if (is_ok()) {
            if (other.is_ok()) {
                this->value = std::forward<Other>(other).get();
            } else {
                replace(result::ErrTag{}, std::forward<Other>(other).get_error());
            }
        } else if (other.is_err()) {
            this->error = std::forward<Other>(other).get_error();
        } else {
            replace(result::OkTag{}, std::forward<Other>(other).get());
        }
    }

    constexpr bool is_ok() const noexcept { return this->state == State::Ok; }
    constexpr bool is_err() const noexcept { return this->state == State::Err; }

    constexpr T& get() & noexcept { return this->value; }
    constexpr const T& get() const& noexcept { return this->value; }
    constexpr T&& get() && noexcept { return std::move(this->value); }
    constexpr const T&& get() const&& noexcept { return std::move(this->value); }

    constexpr E& get_error() & noexcept { return this->error; }
    constexpr const E& get_error() const& noexcept { return this->error; }
    constexpr E&& get_error() && noexcept { return std::move(this->error); }
    constexpr const E&& get_error() const&& noexcept { return std::move(this->error); }

private:
    template <typename U>
    void replace(result::OkTag, U&& incoming) {
        reinitialize(std::addressof(this->value), this->error, std::forward<U>(incoming));
        this->state = State::Ok;
    }

    template <typename U>
    void replace(result::ErrTag, U&& incoming) {
        reinitialize(std::addressof(this->error), this->value, std::forward<U>(incoming));
        this->state = State::Err;
    }

    // Replace the active Old union member with New. If construction throws,
    // preserve or restore the Old alternative.
    template <typename New, typename Old, typename U>
    static void reinitialize(New* dst, Old& prev, U&& incoming) {
        if constexpr (IsNothrowConstructible<New, U&&>{}) {
            // Direct replacement: constructing New from incoming cannot throw,
            // so it is safe to destroy Old first.
            prev.~Old();
            ptr::construct_at(dst, std::forward<U>(incoming));
        } else if constexpr (IsNothrowMoveConstructible<New>{}) {
            // Prepare, then commit: constructing New from incoming may throw,
            // move into temporary before destroying Old.
            New tmp(std::forward<U>(incoming));
            prev.~Old();
            ptr::construct_at(dst, std::move(tmp));
        } else {
            static_assert(
                IsNothrowMoveConstructible<Old>{},
                "Result assignment requires safe rollback"
            );

            // Constructing new may throw so a rollback policy is required to
            // restore Old.
            auto* target = std::addressof(prev);
            Old backup(std::move(prev));
            auto rollback = mem::defer([target, &backup]() noexcept {
                ptr::construct_at(target, std::move(backup));
            });

            prev.~Old();
            ptr::construct_at(dst, std::forward<U>(incoming));
            rollback.cancel();
        }
    }
};

template <typename T, typename E, bool = IsTriviallyCopyConstructible<T, E>{}>
struct CopyBase : Storage<T, E> {
    using Storage<T, E>::Storage;
};

template <typename T, typename E>
struct CopyBase<T, E, false> : Storage<T, E> {
    using Storage<T, E>::Storage;
    CopyBase() = default;

    CopyBase(const CopyBase& other) noexcept(IsNothrowCopyConstructible<T, E>{}) : Storage<T, E>() {
        this->construct_from(other);
    }

    CopyBase(CopyBase&&) = default;
    CopyBase& operator=(const CopyBase&) = default;
    CopyBase& operator=(CopyBase&&) = default;
};

template <
    typename T,
    typename E,
    bool = IsTriviallyMoveConstructible<T, E>{},
    bool = IsMoveConstructible<T, E>{}>
struct MoveBase : CopyBase<T, E> {
    using CopyBase<T, E>::CopyBase;
};

template <typename T, typename E>
struct MoveBase<T, E, false, true> : CopyBase<T, E> {
    using CopyBase<T, E>::CopyBase;
    MoveBase() = default;
    MoveBase(const MoveBase&) = default;

    MoveBase(MoveBase&& other) noexcept(IsNothrowMoveConstructible<T, E>{}) : CopyBase<T, E>() {
        this->construct_from(std::move(other));
    }

    MoveBase& operator=(const MoveBase&) = default;
    MoveBase& operator=(MoveBase&&) = default;
};

template <typename T, typename E>
struct MoveBase<T, E, false, false> : CopyBase<T, E> {
    using CopyBase<T, E>::CopyBase;
    MoveBase() = default;
    MoveBase(const MoveBase&) = default;
    MoveBase(MoveBase&&) = delete;
    MoveBase& operator=(const MoveBase&) = default;
    MoveBase& operator=(MoveBase&&) = default;
};

template <
    typename T,
    typename E,
    bool = IsTriviallyCopyable<T, E>{} && IsTriviallyDestructible<T, E>{}>
struct CopyAssignBase : MoveBase<T, E> {
    using MoveBase<T, E>::MoveBase;
};

template <typename T, typename E>
struct CopyAssignBase<T, E, false> : MoveBase<T, E> {
    using MoveBase<T, E>::MoveBase;
    CopyAssignBase() = default;
    CopyAssignBase(const CopyAssignBase&) = default;
    CopyAssignBase(CopyAssignBase&&) = default;

    CopyAssignBase& operator=(const CopyAssignBase& other) noexcept(IsNothrowCopyable<T, E>{}) {
        this->assign(other);
        return *this;
    }

    CopyAssignBase& operator=(CopyAssignBase&&) = default;
};

template <
    typename T,
    typename E,
    bool = IsTriviallyMovable<T, E>{} && IsTriviallyDestructible<T, E>{}>
struct MoveAssignBase : CopyAssignBase<T, E> {
    using CopyAssignBase<T, E>::CopyAssignBase;
};

template <typename T, typename E>
struct MoveAssignBase<T, E, false> : CopyAssignBase<T, E> {
    using CopyAssignBase<T, E>::CopyAssignBase;
    MoveAssignBase() = default;
    MoveAssignBase(const MoveAssignBase&) = default;
    MoveAssignBase(MoveAssignBase&&) = default;
    MoveAssignBase& operator=(const MoveAssignBase&) = default;

    MoveAssignBase& operator=(MoveAssignBase&& other) noexcept(IsNothrowMovable<T, E>{}) {
        this->assign(std::move(other));
        return *this;
    }
};

} // namespace detail::result::storage

template <typename T, typename E>
class [[nodiscard]] Result final :
    private detail::result::storage::MoveAssignBase<T, E>,
    private special_members::Ctor<T, E>,
    private special_members::Assign<T, E> {
    static_assert(IsObject<T, E>{}, "Result requires owned object types");
    static_assert(!IsArray<T>{} && !IsArray<E>{}, "Result does not support arrays");
    static_assert(
        IsSame<T, RemoveCv<T>>{} && IsSame<E, RemoveCv<E>>{},
        "Result payloads must not be cv-qualified"
    );
    static_assert(IsNothrowDestructible<T, E>{}, "Result requires nonthrowing destructors");

    using Base = detail::result::storage::MoveAssignBase<T, E>;

public:
    using value_type = T;
    using error_type = E;
    static constexpr detail::result::OkTag ok_tag{};
    static constexpr detail::result::ErrTag err_tag{};

    Result() = delete;
    Result(const Result&) = default;
    Result(Result&&) = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) = default;

    template <typename... Args, EnableIf<IsConstructible<T, Args...>{}, int> = 0>
    constexpr explicit Result(detail::result::OkTag tag, Args&&... args) noexcept(
        IsNothrowConstructible<T, Args...>{}
    ) :
        Base(tag, std::forward<Args>(args)...) {}

    template <typename... Args, EnableIf<IsConstructible<E, Args...>{}, int> = 0>
    constexpr explicit Result(detail::result::ErrTag tag, Args&&... args) noexcept(
        IsNothrowConstructible<E, Args...>{}
    ) :
        Base(tag, std::forward<Args>(args)...) {}

    template <
        typename U,
        EnableIf<IsConstructible<T, const U&>{} && IsConvertible<const U&, T>{}, int> = 0>
    constexpr Result(const Ok<U>& wrapped) noexcept(IsNothrowConstructible<T, const U&>{}) :
        Base(ok_tag, wrapped.value) {}

    template <
        typename U,
        EnableIf<IsConstructible<T, const U&>{} && !IsConvertible<const U&, T>{}, int> = 0>
    constexpr explicit Result(const Ok<U>& wrapped) noexcept(
        IsNothrowConstructible<T, const U&>{}
    ) :
        Base(ok_tag, wrapped.value) {}

    template <typename U, EnableIf<IsConstructible<T, U&&>{} && IsConvertible<U&&, T>{}, int> = 0>
    constexpr Result(Ok<U>&& wrapped) noexcept(IsNothrowConstructible<T, U&&>{}) :
        Base(ok_tag, std::forward<U>(wrapped.value)) {}

    template <typename U, EnableIf<IsConstructible<T, U&&>{} && !IsConvertible<U&&, T>{}, int> = 0>
    constexpr explicit Result(Ok<U>&& wrapped) noexcept(IsNothrowConstructible<T, U&&>{}) :
        Base(ok_tag, std::forward<U>(wrapped.value)) {}

    template <
        typename U,
        EnableIf<IsConstructible<E, const U&>{} && IsConvertible<const U&, E>{}, int> = 0>
    constexpr Result(const Err<U>& wrapped) noexcept(IsNothrowConstructible<E, const U&>{}) :
        Base(err_tag, wrapped.error) {}

    template <
        typename U,
        EnableIf<IsConstructible<E, const U&>{} && !IsConvertible<const U&, E>{}, int> = 0>
    constexpr explicit Result(const Err<U>& wrapped) noexcept(
        IsNothrowConstructible<E, const U&>{}
    ) :
        Base(err_tag, wrapped.error) {}

    template <typename U, EnableIf<IsConstructible<E, U&&>{} && IsConvertible<U&&, E>{}, int> = 0>
    constexpr Result(Err<U>&& wrapped) noexcept(IsNothrowConstructible<E, U&&>{}) :
        Base(err_tag, std::forward<U>(wrapped.error)) {}

    template <typename U, EnableIf<IsConstructible<E, U&&>{} && !IsConvertible<U&&, E>{}, int> = 0>
    constexpr explicit Result(Err<U>&& wrapped) noexcept(IsNothrowConstructible<E, U&&>{}) :
        Base(err_tag, std::forward<U>(wrapped.error)) {}

    ~Result() = default;

    template <typename U, EnableIf<IsConstructible<T, U&&>{}, int> = 0>
    [[nodiscard]] static constexpr Result ok(U&& value) noexcept(IsNothrowConstructible<T, U&&>{}) {
        return Result(ok_tag, std::forward<U>(value));
    }

    template <typename U, EnableIf<IsConstructible<E, U&&>{}, int> = 0>
    [[nodiscard]] static constexpr Result err(U&& error) noexcept(
        IsNothrowConstructible<E, U&&>{}
    ) {
        return Result(err_tag, std::forward<U>(error));
    }

    template <typename U>
    [[nodiscard]] constexpr Result<U, E> and_(Result<U, E> other) const& {
        if (is_err()) {
            return Result<U, E>::err(this->error);
        }
        return other;
    }

    template <typename U>
    [[nodiscard]] constexpr Result<U, E> and_(Result<U, E> other) && {
        if (is_err()) {
            return Result<U, E>::err(std::move(this->error));
        }
        return other;
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto and_then(Fn&& fn) & {
        using R = InvokeResult<Fn, T&>;
        static_assert(IsResult<R>{}, "and_then must return Result<U, E>");
        static_assert(IsSame<typename R::error_type, E>{}, "and_then must preserve the error type");

        if (is_err()) {
            return R::err(this->error);
        }
        return std::invoke(std::forward<Fn>(fn), this->value);
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto and_then(Fn&& fn) const& {
        using R = InvokeResult<Fn, const T&>;
        static_assert(IsResult<R>{}, "and_then must return Result<U, E>");
        static_assert(IsSame<typename R::error_type, E>{}, "and_then must preserve the error type");

        if (is_err()) {
            return R::err(this->error);
        }
        return std::invoke(std::forward<Fn>(fn), this->value);
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto and_then(Fn&& fn) && {
        using R = InvokeResult<Fn, T&&>;
        static_assert(IsResult<R>{}, "and_then must return Result<U, E>");
        static_assert(IsSame<typename R::error_type, E>{}, "and_then must preserve the error type");

        if (is_err()) {
            return R::err(std::move(this->error));
        }
        return std::invoke(std::forward<Fn>(fn), std::move(this->value));
    }

    // Mapped alternatives are owned values, including when the callable returns a reference.
    template <typename Fn>
    [[nodiscard]] constexpr auto map(Fn&& fn) & {
        using U = Decay<InvokeResult<Fn, T&>>;
        using R = Result<U, E>;

        if (is_err()) {
            return R::err(this->error);
        }
        return R::ok(std::invoke(std::forward<Fn>(fn), this->value));
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map(Fn&& fn) const& {
        using U = Decay<InvokeResult<Fn, const T&>>;
        using R = Result<U, E>;

        if (is_err()) {
            return R::err(this->error);
        }
        return R::ok(std::invoke(std::forward<Fn>(fn), this->value));
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map(Fn&& fn) && {
        using U = Decay<InvokeResult<Fn, T&&>>;
        using R = Result<U, E>;

        if (is_err()) {
            return R::err(std::move(this->error));
        }
        return R::ok(std::invoke(std::forward<Fn>(fn), std::move(this->value)));
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map_err(Fn&& fn) & {
        using F = Decay<InvokeResult<Fn, E&>>;
        using R = Result<T, F>;

        if (is_ok()) {
            return R::ok(this->value);
        }
        return R::err(std::invoke(std::forward<Fn>(fn), this->error));
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map_err(Fn&& fn) const& {
        using F = Decay<InvokeResult<Fn, const E&>>;
        using R = Result<T, F>;

        if (is_ok()) {
            return R::ok(this->value);
        }
        return R::err(std::invoke(std::forward<Fn>(fn), this->error));
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map_err(Fn&& fn) && {
        using F = Decay<InvokeResult<Fn, E&&>>;
        using R = Result<T, F>;

        if (is_ok()) {
            return R::ok(std::move(this->value));
        }
        return R::err(std::invoke(std::forward<Fn>(fn), std::move(this->error)));
    }

    template <typename U, typename Fn>
    [[nodiscard]] constexpr U map_or(U default_value, Fn&& fn) & {
        if (is_err()) {
            return default_value;
        }
        return std::invoke(std::forward<Fn>(fn), this->value);
    }

    template <typename U, typename Fn>
    [[nodiscard]] constexpr U map_or(U default_value, Fn&& fn) const& {
        if (is_err()) {
            return default_value;
        }
        return std::invoke(std::forward<Fn>(fn), this->value);
    }

    template <typename U, typename Fn>
    [[nodiscard]] constexpr U map_or(U default_value, Fn&& fn) && {
        if (is_err()) {
            return default_value;
        }
        return std::invoke(std::forward<Fn>(fn), std::move(this->value));
    }

    template <typename DefaultFn, typename Fn>
    [[nodiscard]] constexpr decltype(auto) map_or_else(DefaultFn&& default_fn, Fn&& fn) & {
        using U = InvokeResult<Fn, T&>;
        static_assert(
            IsConvertible<InvokeResult<DefaultFn, E&>, U>{},
            "map_or_else fallback must be convertible to the mapped type"
        );

        if (is_err()) {
            return static_cast<U>(std::invoke(std::forward<DefaultFn>(default_fn), this->error));
        }
        return std::invoke(std::forward<Fn>(fn), this->value);
    }

    template <typename DefaultFn, typename Fn>
    [[nodiscard]] constexpr decltype(auto) map_or_else(DefaultFn&& default_fn, Fn&& fn) const& {
        using U = InvokeResult<Fn, const T&>;
        static_assert(
            IsConvertible<InvokeResult<DefaultFn, const E&>, U>{},
            "map_or_else fallback must be convertible to the mapped type"
        );

        if (is_err()) {
            return static_cast<U>(std::invoke(std::forward<DefaultFn>(default_fn), this->error));
        }
        return std::invoke(std::forward<Fn>(fn), this->value);
    }

    template <typename DefaultFn, typename Fn>
    [[nodiscard]] constexpr decltype(auto) map_or_else(DefaultFn&& default_fn, Fn&& fn) && {
        using U = InvokeResult<Fn, T&&>;
        static_assert(
            IsConvertible<InvokeResult<DefaultFn, E&&>, U>{},
            "map_or_else fallback must be convertible to the mapped type"
        );

        if (is_err()) {
            return static_cast<U>(
                std::invoke(std::forward<DefaultFn>(default_fn), std::move(this->error))
            );
        }
        return std::invoke(std::forward<Fn>(fn), std::move(this->value));
    }

    template <typename Fn>
    [[nodiscard]] constexpr decltype(auto) map_or_default(Fn&& fn) & {
        using U = InvokeResult<Fn, T&>;
        static_assert(IsDefaultConstructible<U>{});

        if (is_err()) {
            return U{};
        }
        return std::invoke(std::forward<Fn>(fn), this->value);
    }

    template <typename Fn>
    [[nodiscard]] constexpr decltype(auto) map_or_default(Fn&& fn) const& {
        using U = InvokeResult<Fn, const T&>;
        static_assert(IsDefaultConstructible<U>{});

        if (is_err()) {
            return U{};
        }
        return std::invoke(std::forward<Fn>(fn), this->value);
    }

    template <typename Fn>
    [[nodiscard]] constexpr decltype(auto) map_or_default(Fn&& fn) && {
        using U = InvokeResult<Fn, T&&>;
        static_assert(IsDefaultConstructible<U>{});

        if (is_err()) {
            return U{};
        }
        return std::invoke(std::forward<Fn>(fn), std::move(this->value));
    }

    [[nodiscard]] constexpr Option<T> ok() const& noexcept(IsNothrowCopyConstructible<T>{}) {
        if (is_err()) {
            return NONE;
        }
        return Option<T>(storage::Some{}, this->value);
    }

    [[nodiscard]] constexpr Option<T> ok() && noexcept(IsNothrowMoveConstructible<T>{}) {
        if (is_err()) {
            return NONE;
        }
        return Option<T>(storage::Some{}, std::move(this->value));
    }

    [[nodiscard]] constexpr T& expect(const char* message) & noexcept {
        OXIDE_ASSERT(is_ok(), message);
        return this->value;
    }

    [[nodiscard]] constexpr const T& expect(const char* message) const& noexcept {
        OXIDE_ASSERT(is_ok(), message);
        return this->value;
    }

    [[nodiscard]] constexpr T&& expect(const char* message) && noexcept {
        OXIDE_ASSERT(is_ok(), message);
        return std::move(this->value);
    }

    [[nodiscard]] constexpr const T&& expect(const char* message) const&& noexcept {
        OXIDE_ASSERT(is_ok(), message);
        return std::move(this->value);
    }

    [[nodiscard]] constexpr E& expect_err(const char* message) & noexcept {
        OXIDE_ASSERT(is_err(), message);
        return this->error;
    }

    [[nodiscard]] constexpr const E& expect_err(const char* message) const& noexcept {
        OXIDE_ASSERT(is_err(), message);
        return this->error;
    }

    [[nodiscard]] constexpr E&& expect_err(const char* message) && noexcept {
        OXIDE_ASSERT(is_err(), message);
        return std::move(this->error);
    }

    [[nodiscard]] constexpr const E&& expect_err(const char* message) const&& noexcept {
        OXIDE_ASSERT(is_err(), message);
        return std::move(this->error);
    }

    [[nodiscard]] constexpr T& unwrap() & noexcept {
        return expect("called `Result::unwrap()` on an `Err` value");
    }

    [[nodiscard]] constexpr const T& unwrap() const& noexcept {
        return expect("called `Result::unwrap()` on an `Err` value");
    }

    [[nodiscard]] constexpr T&& unwrap() && noexcept {
        return std::move(*this).expect("called `Result::unwrap()` on an `Err` value");
    }

    [[nodiscard]] constexpr const T&& unwrap() const&& noexcept {
        return std::move(*this).expect("called `Result::unwrap()` on an `Err` value");
    }

    [[nodiscard]] constexpr E& unwrap_err() & noexcept {
        return expect_err("called `Result::unwrap_err()` on an `Ok` value");
    }

    [[nodiscard]] constexpr const E& unwrap_err() const& noexcept {
        return expect_err("called `Result::unwrap_err()` on an `Ok` value");
    }

    [[nodiscard]] constexpr E&& unwrap_err() && noexcept {
        return std::move(*this).expect_err("called `Result::unwrap_err()` on an `Ok` value");
    }

    [[nodiscard]] constexpr const E&& unwrap_err() const&& noexcept {
        return std::move(*this).expect_err("called `Result::unwrap_err()` on an `Ok` value");
    }

    [[nodiscard]] constexpr bool is_ok() const noexcept { return Base::is_ok(); }

    [[nodiscard]] constexpr bool is_err() const noexcept { return Base::is_err(); }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return is_ok(); }
};

} // namespace oxide

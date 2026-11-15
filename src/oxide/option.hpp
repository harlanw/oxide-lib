#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include <oxide/utiliy/special_members.hpp>
#include <oxide/assert.hpp>
#include <oxide/ptr.hpp>
#include <oxide/traits.hpp>
#include <oxide/tuple.hpp>

// clang-format off
#define OXIDE_SOME(name, opt)                                                  \
    OXIDE_SILENCE_DANGLING_ELSE_BEGIN                                          \
    (auto&& OXIDE_MANGLE_NAME(name) = (opt); OXIDE_MANGLE_NAME(name).is_some())\
        if (auto& name = OXIDE_MANGLE_NAME(name).unwrap(); false) {} else      \
    OXIDE_SILENCE_DANGLING_ELSE_END
// clang-format on

namespace oxide {

template <class T>
class Option;

// Result-dependent operations use Result<T, E>::ok/err factories and the
// value_type, error_type, is_ok(), unwrap(), and unwrap_err() interface.
template <typename T, typename E>
class Result;

struct None {
    explicit constexpr None() = default;
};

inline constexpr None NONE{};

template <typename T>
struct IsOption : std::false_type {};

template <typename T>
struct IsOption<Option<T>> : std::true_type {};

namespace storage {

struct Some final {
    explicit constexpr Some() = default;
};

// StorageDtor owns the union and tracks whether T is active.
//
// Separate trivial and non-trivial cases are required because declaring any
// destructor makes StorageDtor itself non-trivially destructible.
template <typename T, bool = IsTriviallyDestructible<T>{}>
struct StorageDtor;

// This is the case where T is not trivially destructible.
template <typename T>
struct StorageDtor<T, false> {
    union {
        None none;
        T value;
    };

    constexpr StorageDtor() noexcept : none{}, has(false) {}

    template <typename... Args>
    constexpr StorageDtor(Some, Args&&... args) noexcept(IsNothrowConstructible<T, Args...>{}) :
        value(std::forward<Args>(args)...),
        has(true) {}

    ~StorageDtor() noexcept {
        if (has) {
            value.~T();
            has = false;
        }
    }

    bool has{false};
};

template <typename T>
struct StorageDtor<T, true> {
    constexpr StorageDtor() noexcept : none{}, has(false) {}

    template <typename... Args>
    constexpr StorageDtor(Some, Args&&... args) noexcept(IsNothrowConstructible<T, Args...>{}) :
        value(std::forward<Args>(args)...),
        has(true) {}

    union {
        None none;
        T value;
    };

    bool has{false};
};

template <typename T>
struct Storage : StorageDtor<T> {
    using StorageDtor<T>::StorageDtor;

    template <typename... Args>
    void construct(Args&&... args) {
        oxide::ptr::construct_at(std::addressof(this->value), std::forward<Args>(args)...);
        this->has = true;
    }

    template <typename U>
    void assign(U&& other) {
        if (is_some()) {
            if (other.is_some()) {
                // T is already alive here, so assign into it rather than
                // destroying and reconstructing it.
                this->value = std::forward<U>(other).get();
            } else {
                this->value.~T();
                this->has = false;
            }
        } else if (other.is_some()) {
            construct(std::forward<U>(other).get());
        }
    }

    void destroy() noexcept {
        this->get().~T();
        this->has = false;
    }

    constexpr bool is_some() const noexcept { return this->has; }
    constexpr bool is_none() const noexcept { return !this->has; }

    constexpr T& get() & { return this->value; }
    constexpr const T& get() const& { return this->value; }
    constexpr T&& get() && { return std::move(this->value); }
    constexpr const T&& get() const&& { return std::move(this->value); }
};

template <typename T, bool = IsTriviallyCopyConstructible<T>{}>
struct CopyBase : Storage<T> {
    using Storage<T>::Storage;
};

// T := not trivially copy constructible
template <class T>
struct CopyBase<T, false> : Storage<T> {
    using Storage<T>::Storage;

    CopyBase() = default;

    CopyBase(const CopyBase& other) : Storage<T>() {
        if (other.is_some()) {
            this->construct(other.get());
        }
    }

    CopyBase(CopyBase&& other) = default;
    CopyBase& operator=(const CopyBase& rhs) = default;
    CopyBase& operator=(CopyBase&& rhs) = default;
};

template <typename T, bool = IsTriviallyMoveConstructible<T>{}>
struct MoveBase : CopyBase<T> {
    using CopyBase<T>::CopyBase;
};

template <typename T>
struct MoveBase<T, false> : CopyBase<T> {
    using CopyBase<T>::CopyBase;

    MoveBase() = default;
    MoveBase(const MoveBase&) = default;

    MoveBase(MoveBase&& other) noexcept(IsNothrowMoveConstructible<T>{}) : CopyBase<T>() {
        if (other.is_some()) {
            this->construct(std::move(other).get());
        }
    }

    MoveBase& operator=(const MoveBase&) = default;
    MoveBase& operator=(MoveBase&&) = default;
};

template <
    typename T,
    bool = IsTriviallyCopyConstructible<T>{}
        && //
        IsTriviallyCopyAssignable<T>{}
        &&                            //
        IsTriviallyDestructible<T>{}> //
struct CopyAssignBase : MoveBase<T> {
    using MoveBase<T>::MoveBase;
};

template <typename T>
struct CopyAssignBase<T, false> : MoveBase<T> {
    using MoveBase<T>::MoveBase;

    CopyAssignBase() = default;
    CopyAssignBase(const CopyAssignBase&) = default;
    CopyAssignBase(CopyAssignBase&&) = default;

    CopyAssignBase& operator=(const CopyAssignBase& other) {
        this->assign(other);
        return *this;
    }

    CopyAssignBase& operator=(CopyAssignBase&&) = default;
};

template <
    typename T,
    bool = IsTriviallyMoveConstructible<T>{}
        && IsTriviallyMoveAssignable<T>{}
        && IsTriviallyDestructible<T>{}>
struct MoveAssignBase : CopyAssignBase<T> {
    using CopyAssignBase<T>::CopyAssignBase;
};

template <typename T>
struct MoveAssignBase<T, false> : CopyAssignBase<T> {
    using CopyAssignBase<T>::CopyAssignBase;

    MoveAssignBase() = default;
    MoveAssignBase(const MoveAssignBase&) = default;
    MoveAssignBase(MoveAssignBase&&) = default;

    MoveAssignBase& operator=(const MoveAssignBase&) = default;

    MoveAssignBase& operator=(MoveAssignBase&& other) noexcept(IsNothrowMovable<T>{}) {
        this->assign(std::move(other));
        return *this;
    }
};

} // namespace storage

template <typename T>
class Option final :
    private storage::MoveAssignBase<T>,
    private special_members::Ctor<T>,
    private special_members::Assign<T> {
    using Base = storage::MoveAssignBase<T>;

public:
    constexpr Option() noexcept = default;

    constexpr Option(None) noexcept {}

    Option(const Option&) = default;
    Option(Option&&) noexcept(IsNothrowMoveConstructible<T>{}) = default;
    Option& operator=(const Option&) = default;
    Option& operator=(Option&&) noexcept(IsNothrowMovable<T>{}) = default;

    template <typename... Args, EnableIf<IsConstructible<T, Args...>{}, int> = 0>
    constexpr explicit Option(storage::Some, Args&&... args) noexcept(
        IsNothrowConstructible<T, Args...>{}
    ) :
        Base(storage::Some{}, std::forward<Args>(args)...) {}

    template <
        typename U = T,
        EnableIf<
            !IsSame<Decay<U>, Option<T>>{} && IsConstructible<T, U&&>{} && IsConvertible<U&&, T>{},
            int> = 0> // NOLINTNEXTLINE
    constexpr Option(U&& value) : Base(storage::Some{}, std::forward<U>(value)) {}

    template <
        typename U = T,
        EnableIf<
            !IsSame<Decay<U>, Option<T>>{} && IsConstructible<T, U&&>{} && !IsConvertible<U&&, T>{},
            int> = 0> // NOLINTNEXTLINE
    constexpr explicit Option(U&& value) : Base(storage::Some{}, std::forward<U>(value)) {}

    template <
        typename U,
        EnableIf< // clang-format off
            IsConstructible<T, const U&>{} &&
            IsConvertible<const U&, T>{},
            int> = 0> // clang-format on
    Option(const Option<U>& other) {
        if (other.is_some()) {
            this->construct(*other);
        }
    }

    template <
        typename U,
        EnableIf< // clang-format off
            IsConstructible<T, const U&>{} &&
            !IsConvertible<const U&, T>{},
            int> = 0> // clang-format on
    explicit Option(const Option<U>& other) {
        if (other.is_some()) {
            this->construct(*other);
        }
    }

    template <
        typename U,
        EnableIf< // clang-format off
            IsConstructible<T, U&&>{} &&
            IsConvertible<U&&, T>{},
            int> = 0> // clang-format on
    Option(Option<U>&& other) {
        if (other.is_some()) {
            this->construct(std::move(*other));
        }
    }

    template <
        typename U,
        EnableIf< // clang-format off
            IsConstructible<T, U&&>{} &&
            !IsConvertible<U&&, T>{},
            int> = 0> // clang-format on
    explicit Option(Option<U>&& other) {
        if (other.is_some()) {
            this->construct(std::move(*other));
        }
    }

    Option& operator=(None) noexcept {
        this->reset();
        return *this;
    }

    template <
        typename U = T,
        EnableIf< // clang-format off
            IsConstructible<T, U&&>{} &&
            IsAssignable<T&, U&&>{},
            int> = 0> // clang-format on
    Option& operator=(U&& value) {
        if (is_some()) {
            this->value = std::forward<U>(value);
        } else {
            this->construct(std::forward<U>(value));
        }

        return *this;
    }

    template <
        typename U,
        EnableIf< // clang-format off
            IsConstructible<T, const U&>{} &&
            IsAssignable<T&, const U&>{},
            int> = 0> // clang-format on
    Option& operator=(const Option<U>& other) {
        if (is_some()) {
            if (other.is_some()) {
                this->value = *other;
            } else {
                this->destroy();
            }
        } else if (other.is_some()) {
            this->construct(*other);
        }

        return *this;
    }

    template <
        typename U,
        EnableIf< // clang-format off
            IsConstructible<T, U&&>{} &&
            IsAssignable<T&, U&&>{},
            int> = 0> // clang-format on
    Option& operator=(Option<U>&& other) {
        if (is_some()) {
            if (other.is_some()) {
                this->value = std::move(*other);
            } else {
                this->destroy();
            }
        } else if (other.is_some()) {
            this->construct(std::move(*other));
        }

        return *this;
    }

    ~Option() = default;

    Option take() {
        Option result = std::move(*this);
        reset();
        return result;
    }

    // Borrow the contained value. The borrowed Option must not outlive *this.
    [[nodiscard]] constexpr Option<const T&> as_ref() const& noexcept {
        return is_some() ? Option<const T&>(this->value) : Option<const T&>(NONE);
    }

    // Do not borrow from a temporary owning Option.
    Option<const T&> as_ref() const&& = delete;

    [[nodiscard]] constexpr Option<T&> as_mut() & noexcept {
        return is_some() ? Option<T&>(this->value) : Option<T&>(NONE);
    }

    T& get_or_insert(T value) {
        if (is_none()) {
            this->construct(std::move(value));
        }
        return this->value;
    }

    template <typename Fn>
    T& get_or_insert_with(Fn&& fn) {
        if (is_none()) {
            this->construct(std::invoke(std::forward<Fn>(fn)));
        }
        return this->value;
    }

    template <typename U = T, EnableIf<IsDefaultConstructible<U>{}, int> = 0>
    T& get_or_insert_default() {
        if (is_none()) {
            this->construct();
        }
        return this->value;
    }

    T& insert(T value) {
        reset();
        this->construct(std::move(value));
        return this->value;
    }

    [[nodiscard]] Option replace(T value) {
        Option old = take();
        this->construct(std::move(value));
        return old;
    }

    template <typename Fn>
    [[nodiscard]] Option take_if(Fn&& fn) {
        if (is_some() && std::invoke(std::forward<Fn>(fn), this->value)) {
            return take();
        }
        return NONE;
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map(Fn&& function) & {
        using Result = InvokeResult<Fn, T&>;
        using Mapped = Conditional<IsLvalueRef<Result>{}, Result, Decay<Result>>;

        if (hints::unlikely(is_none())) {
            return Option<Mapped>(NONE);
        }

        return Option<Mapped>(std::invoke(std::forward<Fn>(function), unwrap()));
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map(Fn&& function) const& {
        using Result = InvokeResult<Fn, const T&>;
        using Mapped = Conditional<IsLvalueRef<Result>{}, Result, Decay<Result>>;

        if (hints::unlikely(is_none())) {
            return Option<Mapped>(NONE);
        }

        return Option<Mapped>(std::invoke(std::forward<Fn>(function), unwrap()));
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map(Fn&& function) && {
        using Result = InvokeResult<Fn, T&&>;
        using Mapped = Conditional<IsLvalueRef<Result>{}, Result, Decay<Result>>;

        if (hints::unlikely(is_none())) {
            return Option<Mapped>(NONE);
        }

        return Option<Mapped>(std::invoke(std::forward<Fn>(function), std::move(*this).unwrap()));
    }

    template <typename U, typename Fn>
    [[nodiscard]] constexpr U map_or(U default_value, Fn&& function) & {
        if (hints::unlikely(is_none())) {
            return default_value;
        }

        return std::invoke(std::forward<Fn>(function), unwrap());
    }

    template <typename U, typename Fn>
    [[nodiscard]] constexpr U map_or(U default_value, Fn&& function) const& {
        if (hints::unlikely(is_none())) {
            return default_value;
        }

        return std::invoke(std::forward<Fn>(function), unwrap());
    }

    template <typename U, typename Fn>
    [[nodiscard]] constexpr U map_or(U default_value, Fn&& function) && {
        if (hints::unlikely(is_none())) {
            return default_value;
        }

        return std::invoke(std::forward<Fn>(function), std::move(*this).unwrap());
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map_or_default(Fn&& fn) const& -> InvokeResult<Fn, const T&> {
        using U = InvokeResult<Fn, const T&>;
        static_assert(IsDefaultConstructible<U>{});

        if (is_none()) {
            return U{};
        }
        return std::invoke(std::forward<Fn>(fn), this->value);
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map_or_default(Fn&& fn) && -> InvokeResult<Fn, T&&> {
        using U = InvokeResult<Fn, T&&>;
        static_assert(IsDefaultConstructible<U>{});

        if (is_none()) {
            return U{};
        }
        return std::invoke(std::forward<Fn>(fn), std::move(this->value));
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto and_then(Fn&& fn) & {
        using R = InvokeResult<Fn, T&>;

        static_assert(IsOption<R>{}, "and_then must return Option<U>");

        if (hints::unlikely(is_none())) {
            return R(NONE);
        }

        return std::invoke(std::forward<Fn>(fn), unwrap());
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto and_then(Fn&& fn) const& {
        using R = InvokeResult<Fn, const T&>;

        static_assert(IsOption<R>{}, "and_then must return Option<U>");

        if (hints::unlikely(is_none())) {
            return R(NONE);
        }

        return std::invoke(std::forward<Fn>(fn), unwrap());
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto and_then(Fn&& fn) && {
        using R = InvokeResult<Fn, T&&>;

        static_assert(IsOption<R>{}, "and_then must return Option<U>");

        if (hints::unlikely(is_none())) {
            return R(NONE);
        }

        return std::invoke(std::forward<Fn>(fn), std::move(*this).unwrap());
    }

    template <typename Fn>
    [[nodiscard]] constexpr Option filter(Fn&& fn) && {
        if (is_some() && std::invoke(std::forward<Fn>(fn), std::as_const(this->value))) {
            return std::move(*this);
        }
        return Option(NONE);
    }

    template <typename Fn>
    [[nodiscard]] constexpr Option inspect(Fn&& fn) && {
        if (is_some()) {
            std::invoke(std::forward<Fn>(fn), std::as_const(this->value));
        }
        return std::move(*this);
    }

    [[nodiscard]] constexpr Option or_(Option other) && {
        return is_some() ? std::move(*this) : std::move(other);
    }

    template <typename Fn>
    [[nodiscard]] constexpr Option or_else(Fn&& fn) && {
        return is_some() ? std::move(*this) : std::invoke(std::forward<Fn>(fn));
    }

    template <typename U>
    [[nodiscard]] constexpr Option<U> and_(Option<U> other) && {
        return is_some() ? std::move(other) : Option<U>(NONE);
    }

    [[nodiscard]] constexpr Option xor_(Option other) && {
        if (is_some() == other.is_some()) {
            return Option(NONE);
        }
        return is_some() ? std::move(*this) : std::move(other);
    }

    template <typename E>
    [[nodiscard]] constexpr Result<T, E> ok_or(E error) && {
        using R = Result<T, E>;
        if (is_none()) {
            return R::err(std::forward<E>(error));
        }
        return R::ok(std::move(*this).unwrap());
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto ok_or_else(Fn&& fn) && {
        using Error = InvokeResult<Fn>;
        using E = Conditional<IsLvalueRef<Error>{}, Error, Decay<Error>>;
        using R = Result<T, E>;
        if (is_none()) {
            return R::err(std::invoke(std::forward<Fn>(fn)));
        }
        return R::ok(std::move(*this).unwrap());
    }

    template <
        typename U = RemoveCv<T>,
        EnableIf<IsSame<U, RemoveCv<T>>{} && IsOption<U>{}, int> = 0>
    [[nodiscard]] constexpr U flatten() && {
        if (is_none()) {
            return U(NONE);
        }
        return std::move(*this).unwrap();
    }

    // template <
    //     typename U = RemoveCv<T>,
    //     EnableIf<IsSame<U, RemoveCv<T>>{} && detail::IsResult<U>{}, int> = 0>
    // [[nodiscard]] constexpr auto transpose() && {
    //     using V = typename U::value_type;
    //     using E = typename U::error_type;
    //     using R = Result<Option<V>, E>;
    //     if (is_none()) {
    //         return R::ok(Option<V>(NONE));
    //     }
    //     auto&& inner = std::move(*this).unwrap();
    //     if (inner.is_ok()) {
    //         if constexpr (IsLvalueRef<V>{}) {
    //             return R::ok(Option<V>(std::move(inner).unwrap()));
    //         } else {
    //             return R::ok(Option<V>(storage::Some{}, std::move(inner).unwrap()));
    //         }
    //     }
    //     return R::err(std::move(inner).unwrap_err());
    // }

    template <typename U>
    [[nodiscard]] constexpr Option<Pair<T, U>> zip(Option<U> other) && {
        if (is_none() || other.is_none()) {
            return NONE;
        }
        return Option<
            Pair<T, U>>(storage::Some{}, std::move(*this).unwrap(), std::move(other).unwrap());
    }

    constexpr T& expect(const char* message) & noexcept {
        OXIDE_ASSERT(is_some(), message);
        return this->value;
    }

    constexpr const T& expect(const char* message) const& noexcept {
        OXIDE_ASSERT(is_some(), message);
        return this->value;
    }

    constexpr T&& expect(const char* message) && noexcept {
        OXIDE_ASSERT(is_some(), message);
        return std::move(this->value);
    }

    constexpr const T&& expect(const char* message) const&& noexcept {
        OXIDE_ASSERT(is_some(), message);
        return std::move(this->value);
    }

    constexpr T& unwrap() & noexcept {
        return expect("called `Option::unwrap()` on a `None` value");
    }

    constexpr const T& unwrap() const& noexcept {
        return expect("called `Option::unwrap()` on a `None` value");
    }

    constexpr T&& unwrap() && noexcept {
        return std::move(*this).expect("called `Option::unwrap()` on a `None` value");
    }

    constexpr const T&& unwrap() const&& noexcept {
        return std::move(*this).expect("called `Option::unwrap()` on a `None` value");
    }

    template <typename U>
    [[nodiscard]] constexpr T unwrap_or(U&& fallback) const& {
        return hints::likely(is_some()) ? unwrap() : static_cast<T>(std::forward<U>(fallback));
    }

    template <typename U>
    [[nodiscard]] constexpr T unwrap_or(U&& fallback) && {
        return hints::likely(is_some())
            ? std::move(*this).unwrap()
            : static_cast<T>(std::forward<U>(fallback));
    }

    template <typename Fn>
    [[nodiscard]] constexpr T unwrap_or_else(Fn&& fn) const& {
        return hints::likely(is_some()) ? unwrap() : T(std::invoke(std::forward<Fn>(fn)));
    }

    template <typename Fn>
    [[nodiscard]] constexpr T unwrap_or_else(Fn&& fn) && {
        return hints::likely(is_some())
            ? std::move(*this).unwrap()
            : T(std::invoke(std::forward<Fn>(fn)));
    }

    [[nodiscard]] constexpr T& unwrap_unchecked() & noexcept { return this->value; }

    [[nodiscard]] constexpr const T& unwrap_unchecked() const& noexcept { return this->value; }

    [[nodiscard]] constexpr T&& unwrap_unchecked() && noexcept { return std::move(this->value); }

    [[nodiscard]] constexpr const T&& unwrap_unchecked() const&& noexcept {
        return std::move(this->value);
    }

    [[nodiscard]] constexpr bool is_some() const noexcept { return Base::is_some(); }

    [[nodiscard]] constexpr bool is_none() const noexcept { return Base::is_none(); }

    constexpr T* operator->() noexcept { return std::addressof(unwrap()); }

    constexpr const T* operator->() const noexcept { return std::addressof(unwrap()); }

    constexpr T& operator*() & noexcept { return unwrap(); }

    constexpr const T& operator*() const& noexcept { return unwrap(); }

    constexpr T&& operator*() && noexcept { return std::move(*this).unwrap(); }

    constexpr const T&& operator*() const&& noexcept { return std::move(*this).unwrap(); }

    friend constexpr bool operator==(const Option& option, const T& value) {
        return option.is_some() && option.unwrap() == value;
    }

    friend constexpr bool operator==(const T& value, const Option& option) {
        return option.is_some() && value == option.unwrap();
    }

private:
    void reset() noexcept {
        if (is_some()) {
            this->destroy();
        }
    }
};

template <typename T>
class Option<T&> final {
public:
    using value_type = T&;

    constexpr Option() noexcept = default;

    constexpr Option(None) noexcept {}

    Option(const Option&) noexcept = default;
    Option(Option&&) noexcept = default;
    Option& operator=(const Option&) noexcept = default;
    Option& operator=(Option&&) noexcept = default;

    Option& operator=(None) noexcept {
        _value = nullptr;
        return *this;
    }

    constexpr Option(T& value) noexcept : _value(std::addressof(value)) {}
    Option& operator=(T& value) noexcept {
        _value = std::addressof(value);
        return *this;
    }

    // A borrowed option may only bind to an lvalue; accepting a temporary
    // would leave _value dangling as soon as the full expression ends.
    Option(T&&) = delete;
    Option& operator=(T&&) = delete;

    [[nodiscard]] Option take() noexcept {
        Option result = *this;
        _value = nullptr;
        return result;
    }

    template <
        typename U = RemoveCv<T>,
        EnableIf<IsTriviallyCopyConstructible<U>{} && IsConstructible<U, T&>{}, int> = 0>
    [[nodiscard]] constexpr Option<U> copied() const {
        if (is_none()) {
            return NONE;
        }
        return Option<U>(*_value);
    }

    template <
        typename U = RemoveCv<T>,
        EnableIf<IsCopyConstructible<U>{} && IsConstructible<U, T&>{}, int> = 0>
    [[nodiscard]] constexpr Option<U> cloned() const {
        if (is_none()) {
            return NONE;
        }
        return Option<U>(*_value);
    }

    // An Option<T&> does not own its referent, so borrowing a temporary
    // reference wrapper is safe as long as the referent itself is alive.
    [[nodiscard]] constexpr Option<const T&> as_ref() const noexcept {
        return is_some() ? Option<const T&>(*_value) : Option<const T&>(NONE);
    }

    [[nodiscard]] constexpr Option<T&> as_mut() const noexcept { return *this; }

    constexpr void rebind(T& value) noexcept { _value = std::addressof(value); }

    template <typename Fn>
    [[nodiscard]] constexpr auto map(Fn&& fn) const {
        using U = InvokeResult<Fn, T&>;
        using R = Conditional<IsLvalueRef<U>{}, U, Decay<U>>;

        if (hints::unlikely(is_none())) {
            return Option<R>(NONE);
        }

        return Option<R>(std::invoke(std::forward<Fn>(fn), unwrap()));
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map_or_default(Fn&& fn) const -> InvokeResult<Fn, T&> {
        using U = InvokeResult<Fn, T&>;
        static_assert(IsDefaultConstructible<U>{});

        if (is_none()) {
            return U{};
        }
        return std::invoke(std::forward<Fn>(fn), *_value);
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto and_then(Fn&& fn) const {
        using R = InvokeResult<Fn, T&>;
        static_assert(IsOption<R>{}, "and_then must return Option<U>");

        if (hints::unlikely(is_none())) {
            return R(NONE);
        }

        return std::invoke(std::forward<Fn>(fn), unwrap());
    }

    template <typename Fn>
    [[nodiscard]] constexpr Option filter(Fn&& fn) const {
        if (is_some() && std::invoke(std::forward<Fn>(fn), std::as_const(*_value))) {
            return *this;
        }
        return NONE;
    }

    template <typename Fn>
    [[nodiscard]] constexpr Option inspect(Fn&& fn) const {
        if (is_some()) {
            std::invoke(std::forward<Fn>(fn), std::as_const(*_value));
        }
        return *this;
    }

    [[nodiscard]] constexpr Option or_(Option other) const noexcept {
        return is_some() ? *this : other;
    }

    template <typename Fn>
    [[nodiscard]] constexpr Option or_else(Fn&& fn) const {
        return is_some() ? *this : std::invoke(std::forward<Fn>(fn));
    }

    template <typename U>
    [[nodiscard]] constexpr Option<U> and_(Option<U> other) const {
        return is_some() ? std::move(other) : Option<U>(NONE);
    }

    [[nodiscard]] constexpr Option xor_(Option other) const noexcept {
        if (is_some() == other.is_some()) {
            return NONE;
        }
        return is_some() ? *this : other;
    }

    template <typename E>
    [[nodiscard]] constexpr Result<T&, E> ok_or(E error) const {
        using R = Result<T&, E>;
        if (is_none()) {
            return R::err(std::forward<E>(error));
        }
        return R::ok(unwrap());
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto ok_or_else(Fn&& fn) const {
        using Error = InvokeResult<Fn>;
        using E = Conditional<IsLvalueRef<Error>{}, Error, Decay<Error>>;
        using R = Result<T&, E>;
        if (is_none()) {
            return R::err(std::invoke(std::forward<Fn>(fn)));
        }
        return R::ok(unwrap());
    }

    template <
        typename U = RemoveCv<T>,
        EnableIf<IsSame<U, RemoveCv<T>>{} && IsOption<U>{}, int> = 0>
    [[nodiscard]] constexpr U flatten() const {
        if (is_none()) {
            return U(NONE);
        }
        return unwrap();
    }

    // template <
    //     typename U = RemoveCv<T>,
    //     EnableIf<IsSame<U, RemoveCv<T>>{} && detail::IsResult<U>{}, int> = 0>
    // [[nodiscard]] constexpr auto transpose() const {
    //     using V = typename U::value_type;
    //     using E = typename U::error_type;
    //     using R = Result<Option<V>, E>;
    //     if (is_none()) {
    //         return R::ok(Option<V>(NONE));
    //     }
    //     auto& inner = unwrap();
    //     if (inner.is_ok()) {
    //         if constexpr (IsLvalueRef<V>{}) {
    //             return R::ok(Option<V>(inner.unwrap()));
    //         } else {
    //             return R::ok(Option<V>(storage::Some{}, inner.unwrap()));
    //         }
    //     }
    //     return R::err(inner.unwrap_err());
    // }

    template <typename U>
    [[nodiscard]] constexpr Option<std::pair<T&, U>> zip(Option<U> other) const {
        if (is_none() || other.is_none()) {
            return NONE;
        }
        return Option<std::pair<T&, U>>(storage::Some{}, unwrap(), std::move(other).unwrap());
    }

    constexpr T& expect(const char* message) const noexcept {
        OXIDE_ASSERT(is_some(), message);
        return *_value;
    }

    constexpr T& unwrap() const noexcept {
        return expect("called `Option::unwrap()` on a `None` value");
    }

    template <
        typename U,
        EnableIf<IsLvalueRef<U&&>{} && IsConvertible<AddPtr<RemoveRef<U>>, T*>{}, int> = 0>
    [[nodiscard]] constexpr T& unwrap_or(U&& fallback) const noexcept {
        return hints::likely(is_some()) ? unwrap() : fallback;
    }

    template <
        typename Fn,
        typename R = InvokeResult<Fn>,
        EnableIf<IsLvalueRef<R>{} && IsConvertible<AddPtr<RemoveRef<R>>, T*>{}, int> = 0>
    [[nodiscard]] constexpr T& unwrap_or_else(Fn&& fn) const {
        return hints::likely(is_some()) ? unwrap() : std::invoke(std::forward<Fn>(fn));
    }

    [[nodiscard]] constexpr T& unwrap_unchecked() const noexcept { return *_value; }

    [[nodiscard]] constexpr bool is_some() const noexcept { return _value != nullptr; }

    [[nodiscard]] constexpr bool is_none() const noexcept { return _value == nullptr; }

    constexpr T* operator->() const noexcept { return std::addressof(unwrap()); }

    constexpr T& operator*() const noexcept { return unwrap(); }

    friend constexpr bool operator==(const Option& option, const RemoveCv<T>& value) {
        return option.is_some() && option.unwrap() == value;
    }

    friend constexpr bool operator==(const RemoveCv<T>& value, const Option& option) {
        return option.is_some() && value == option.unwrap();
    }

private:
    T* _value{nullptr};
};

template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(const Option<T>& a, const Option<U>& b) {
    return a.is_none() ? b.is_none() : (b.is_some() && *a == *b);
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator!=(const Option<T>& a, const Option<U>& b) {
    return !(a == b);
}

template <typename T>
[[nodiscard]] constexpr bool operator==(const Option<T>& a, None) noexcept {
    return a.is_none();
}

template <typename T>
[[nodiscard]] constexpr bool operator==(None, const Option<T>& a) noexcept {
    return a.is_none();
}

template <typename T>
[[nodiscard]] constexpr bool operator!=(const Option<T>& a, None) noexcept {
    return a.is_some();
}

template <typename T>
[[nodiscard]] constexpr bool operator!=(None, const Option<T>& a) noexcept {
    return a.is_some();
}

template <typename T>
[[nodiscard]]
constexpr Option<Decay<T>> some(T&& value) {
    return Option<Decay<T>>(std::forward<T>(value));
}

template <typename T>
[[nodiscard]]
constexpr Option<T&> some_ref(T& value) noexcept {
    return Option<T&>(value);
}

} // namespace oxide

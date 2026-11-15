#pragma once

#include <functional>
#include <utility>

#include <oxide/assert.hpp>
#include <oxide/option.hpp>
#include <oxide/traits.hpp>

namespace oxide::iter {

namespace detail {

template <typename Source, typename Fn>
struct Map {
    Source source;
    Fn function;

    using Input = decltype(std::declval<Source&>().read());
    using Output = InvokeResult<Fn&, Input>;
    // A reference into a temporary mapped input must be materialized before
    // that input is destroyed. References into borrowed inputs remain borrowed.
    using Item = Conditional<IsLvalueRef<Input, Output>{}, Output, Decay<Output>>;
    static_assert(!IsVoid<Item>{}, "Iterator::map must return an item");

    [[nodiscard]] constexpr bool empty() const noexcept(noexcept(source.empty())) {
        return source.empty();
    }

    [[nodiscard]] constexpr Item read() { return std::invoke(function, source.read()); }

    constexpr void advance() noexcept(noexcept(source.advance())) { source.advance(); }
};

} // namespace detail

// A consuming range over a source with empty(), read(), and advance(). Sources
// must keep borrowed items alive after advance(). Mapping stores its callable
// directly and creates no intermediate container. Borrowed storage must outlive
// iteration.
template <typename Source>
class Iterator final {
    Source _source;

public:
    using item_type = decltype(std::declval<Source&>().read());
    using Next = Option<Conditional<IsLvalueRef<item_type>{}, item_type, Decay<item_type>>>;

    explicit constexpr Iterator(Source source) noexcept(IsNothrowMoveConstructible<Source>{}) :
        _source(std::move(source)) {}

    [[nodiscard]] constexpr Next next() {
        if (_source.empty()) {
            return NONE;
        }

        if constexpr (IsLvalueRef<item_type>{}) {
            Next item(_source.read());
            _source.advance();
            return item;
        } else {
            Next item(storage::Some{}, _source.read());
            _source.advance();
            return item;
        }
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map(Fn&& fn) const& {
        using Mapped = detail::Map<Source, Decay<Fn>>;
        return Iterator<Mapped>(Mapped{_source, std::forward<Fn>(fn)});
    }

    template <typename Fn>
    [[nodiscard]] constexpr auto map(Fn&& fn) && {
        using Mapped = detail::Map<Source, Decay<Fn>>;
        return Iterator<Mapped>(Mapped{std::move(_source), std::forward<Fn>(fn)});
    }

    // T must be default-constructible and provide push(item).
    template <typename T>
    [[nodiscard]] constexpr T collect() {
        T result{};
        for (;;) {
            auto item = next();
            if (item.is_none()) {
                return result;
            }
            result.push(std::move(item).unwrap());
        }
    }

    struct Sentinel {};

    class Cursor final {
        Iterator* _iterator;

    public:
        explicit constexpr Cursor(Iterator& iterator) noexcept : _iterator(&iterator) {}

        [[nodiscard]] constexpr decltype(auto) operator*() const {
            OXIDE_ASSERT(!_iterator->_source.empty(), "dereferenced an exhausted Iterator");
            return _iterator->_source.read();
        }

        constexpr Cursor& operator++() {
            OXIDE_ASSERT(!_iterator->_source.empty(), "advanced an exhausted Iterator");
            _iterator->_source.advance();
            return *this;
        }

        [[nodiscard]] constexpr bool operator==(Sentinel) const {
            return _iterator->_source.empty();
        }

        [[nodiscard]] constexpr bool operator!=(Sentinel sentinel) const {
            return !(*this == sentinel);
        }
    };

    // Cursors borrow this Iterator. begin() does not rewind it, and mapping is
    // evaluated on each dereference rather than when advancing the cursor.
    [[nodiscard]] constexpr Cursor begin() & noexcept { return Cursor(*this); }

    [[nodiscard]] constexpr Sentinel end() const noexcept { return {}; }
};

} // namespace oxide::iter

#pragma once

#include <utility>

#include <oxide/compiler/hints.hpp>
#include <oxide/option.hpp>
#include <oxide/traits.hpp>

namespace oxide::mem {

template <typename Fn>
class ScopeGuard final {
public:
    explicit ScopeGuard(const Fn& fn) noexcept : _fn(fn) {}

    explicit ScopeGuard(Fn&& fn) noexcept : _fn(std::move(fn)) {}

    ScopeGuard(ScopeGuard&& other) noexcept : _fn(other._fn.take()) {}

    OXIDE_DISABLE_COPY(ScopeGuard);

    ~ScopeGuard() noexcept { run(); }

    void run() {
        // Move the functor to local scope to ensure it is not left dangling if
        // we throw an exception.
        auto fn = _fn.take();
        if (fn.is_some()) {
            fn.unwrap()();
        }
    }

    void cancel() noexcept { hints::discard(_fn.take()); }

private:
    Option<Fn> _fn;
};

template <typename Fn>
[[nodiscard]]
auto defer(Fn&& fn) noexcept {
    return ScopeGuard<Decay<Fn>>(std::forward<Fn>(fn));
}

// This is safe because, within each category of storage classes (except dynamic
// storage), the compiler will destruct objects in the reverse order of construction.
#define OXIDE_DEFER(code) \
    [[maybe_unused]] auto OXIDE_MANGLE_NAME(defer_) = oxide::scope::defer([&]() { \
        code; \
    })

} // namespace oxide::mem

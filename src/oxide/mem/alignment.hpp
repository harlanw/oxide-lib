#pragma once

namespace oxide {

class Alignment {
public:
    [[nodiscard]] static constexpr Option<Alignment> try_from(usize align) noexcept {
        if (oxide::is_pow2(align)) {
            return some(Alignment::unsafe::from(align));
        }
        return NONE;
    }

    template <typename T>
    [[nodiscard]] static constexpr Alignment from() noexcept {
        return Alignment::unsafe::from(alignof(T));
    }

    [[nodiscard]] constexpr std::align_val_t value() const noexcept { return _inner; }

    [[nodiscard]] constexpr usize as_usize() const noexcept { return static_cast<usize>(_inner); }

    struct unsafe {
        ~unsafe() = delete;

        [[nodiscard]] static constexpr Alignment from(usize align) noexcept {
            return Alignment(std::align_val_t{align});
        }
    };

private:
    std::align_val_t _inner;

    constexpr Alignment(std::align_val_t inner) : _inner(inner) {}
};

enum class LayoutError : u8 {
    InvalidAlignment,
    SizeOverflow,
};

} // namespace oxide

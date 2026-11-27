#pragma once

// The 64-bit FNV-1a hash (Fowler, Noll and Vo): each byte enters the state with an exclusive or, followed by
// a multiplication by the prime. It needs no table and no memory, which suits a digest of a state that
// changes every tick. It is not a cryptographic hash: it finds accidental differences, not deliberate ones.
// Numbers enter in little-endian order, one byte at a time, so the result is the same on every system.

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace nexum::core {

class Fnv1a64 {
public:
    static constexpr std::uint64_t kOffsetBasis = 0xCBF29CE484222325ULL;
    static constexpr std::uint64_t kPrime = 0x00000100000001B3ULL;

    constexpr void AddByte(std::byte value) noexcept {
        m_value = (m_value ^ std::to_integer<std::uint64_t>(value)) * kPrime;
    }

    constexpr void Add(std::span<const std::byte> bytes) noexcept {
        for (const std::byte value : bytes) {
            AddByte(value);
        }
    }

    // The bytes of the text, without a terminator.
    constexpr void Add(std::string_view text) noexcept {
        for (const char character : text) {
            AddByte(static_cast<std::byte>(character));
        }
    }

    // The bytes of the number, the lowest first. A negative number enters as its two's complement.
    template <std::integral Number>
        requires(!std::same_as<Number, bool>)
    constexpr void AddLittleEndian(Number value) noexcept {
        using Bits = std::make_unsigned_t<Number>;
        const auto bits = static_cast<Bits>(value);
        for (std::size_t index = 0; index < sizeof(Bits); ++index) {
            AddByte(static_cast<std::byte>(bits >> (index * 8U)));
        }
    }

    [[nodiscard]] constexpr std::uint64_t Value() const noexcept { return m_value; }

private:
    std::uint64_t m_value = kOffsetBasis;
};

[[nodiscard]] constexpr std::uint64_t Fnv1a64Of(std::span<const std::byte> bytes) noexcept {
    Fnv1a64 hash;
    hash.Add(bytes);
    return hash.Value();
}

[[nodiscard]] constexpr std::uint64_t Fnv1a64Of(std::string_view text) noexcept {
    Fnv1a64 hash;
    hash.Add(text);
    return hash.Value();
}

} // namespace nexum::core

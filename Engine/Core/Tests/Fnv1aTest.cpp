#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include <doctest/doctest.h>

#include "Nexum/Core/Fnv1a.hpp"

using nexum::core::Fnv1a64;
using nexum::core::Fnv1a64Of;

namespace {

// The test vectors of the 64-bit FNV-1a in the FNV document of the IETF (draft-eastlake-fnv).
constexpr std::uint64_t kEmptyText = 0xCBF29CE484222325ULL;
constexpr std::uint64_t kLetterA = 0xAF63DC4C8601EC8CULL;
constexpr std::uint64_t kFoobar = 0x85944171F73967E8ULL;

static_assert(Fnv1a64Of(std::string_view{"foobar"}) == kFoobar, "the hash is computed at compile time too");

} // namespace

TEST_CASE("[0002-CA-06] the 64-bit FNV-1a gives the published test values") {
    CHECK(Fnv1a64Of(std::string_view{""}) == kEmptyText);
    CHECK(Fnv1a64Of(std::string_view{"a"}) == kLetterA);
    CHECK(Fnv1a64Of(std::string_view{"foobar"}) == kFoobar);
    CHECK(Fnv1a64{}.Value() == kEmptyText);
}

TEST_CASE("[0002-CA-06] the hash of bytes is the hash of the same text") {
    constexpr std::array<std::byte, 6> kBytes{std::byte{'f'}, std::byte{'o'}, std::byte{'o'},
                                              std::byte{'b'}, std::byte{'a'}, std::byte{'r'}};

    CHECK(Fnv1a64Of(std::span<const std::byte>{kBytes}) == kFoobar);
}

TEST_CASE("[0002-CA-06] the hash may be fed in parts") {
    Fnv1a64 hash;
    hash.Add(std::string_view{"foo"});
    hash.Add(std::string_view{""});
    hash.AddByte(std::byte{'b'});
    hash.Add(std::string_view{"ar"});

    CHECK(hash.Value() == kFoobar);
}

TEST_CASE("[0002-CA-06] numbers enter the hash in little-endian order, with their size") {
    // 0x64636261 is "abcd" with the lowest byte first.
    Fnv1a64 number;
    number.AddLittleEndian(std::uint32_t{0x64636261U});
    CHECK(number.Value() == Fnv1a64Of(std::string_view{"abcd"}));

    Fnv1a64 smallNumbers;
    smallNumbers.AddLittleEndian(std::uint8_t{0x61U});
    smallNumbers.AddLittleEndian(std::uint16_t{0x6362U});
    smallNumbers.AddLittleEndian(std::uint64_t{0x6A69686766656464ULL});
    CHECK(smallNumbers.Value() == Fnv1a64Of(std::string_view{"abcddefghij"}));

    // The same value with another size is another hash.
    Fnv1a64 wider;
    wider.AddLittleEndian(std::uint64_t{0x64636261U});
    CHECK(wider.Value() != number.Value());
}

TEST_CASE("[0002-CA-06] a negative number enters the hash as its two's complement bytes") {
    constexpr std::array<std::byte, 2> kAllOnes{std::byte{0xFF}, std::byte{0xFF}};

    Fnv1a64 hash;
    hash.AddLittleEndian(std::int16_t{-1});

    CHECK(hash.Value() == Fnv1a64Of(std::span<const std::byte>{kAllOnes}));
}

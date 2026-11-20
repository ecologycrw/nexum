#include <doctest/doctest.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>

#include "Nexum/Core/FrameArena.hpp"

using nexum::core::FrameArena;

namespace {

// The address as a number, to check its alignment.
[[nodiscard]] std::uintptr_t AddressOf(const void* pointer) {
    return std::bit_cast<std::uintptr_t>(pointer);
}

struct Point {
    float x = 1.0F;
    float y = 2.0F;
};

// Aligned to more than its fields need, like the data the GPU reads. Four floats fill it, with no padding.
struct alignas(16) Vector4 {
    float values[4];
};

} // namespace

TEST_CASE("[0000-CA-04] the arena hands out memory with the alignment asked for") {
    FrameArena arena{1024};
    // One byte first, so that the next piece does not start aligned by chance.
    REQUIRE(arena.Allocate(1, 1).has_value());

    for (const std::size_t alignment : {1U, 2U, 4U, 8U, 16U, 32U, 64U, 128U}) {
        CAPTURE(alignment);
        const auto piece = arena.Allocate(3, alignment);
        REQUIRE(piece.has_value());
        CHECK(piece->size() == 3);
        CHECK(AddressOf(piece->data()) % alignment == 0);
    }
}

TEST_CASE("[0000-CA-04] the arena returns an error when it runs out of room") {
    FrameArena arena{64};
    REQUIRE(arena.Allocate(60, 1).has_value());

    const auto refused = arena.Allocate(8, 1);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message ==
          "the frame arena is out of room: 8 bytes asked with alignment 1, 4 of 64 left");
    CHECK(arena.Used() == 60);

    // What is left can still be used.
    CHECK(arena.Allocate(4, 1).has_value());
    CHECK(arena.Used() == 64);
}

TEST_CASE("[0000-CA-04] the padding of the alignment counts against the room of the arena") {
    FrameArena arena{64};
    REQUIRE(arena.Allocate(1, 1).has_value());
    // 60 bytes would fit, but not after the padding up to the next multiple of 16.
    CHECK_FALSE(arena.Allocate(60, 16).has_value());
    CHECK(arena.Used() == 1);
}

TEST_CASE("[0000-CA-04] the arena goes back to zero at every frame") {
    FrameArena arena{256};
    const auto first = arena.Allocate(200, 8);
    REQUIRE(first.has_value());
    const std::byte* const start = first->data();

    for (int frame = 0; frame < 1'000; ++frame) {
        CAPTURE(frame);
        arena.Reset();
        REQUIRE(arena.Used() == 0);
        const auto piece = arena.Allocate(200, 8);
        REQUIRE(piece.has_value());
        CHECK(piece->data() == start);
        CHECK_FALSE(arena.Allocate(100, 1).has_value());
    }
}

TEST_CASE("[0000-CA-04] typed arrays are built in place and aligned for their type") {
    FrameArena arena{1024};
    REQUIRE(arena.Allocate(1, 1).has_value());

    const auto points = arena.AllocateArray<Point>(3);
    REQUIRE(points.has_value());
    REQUIRE(points->size() == 3);
    CHECK(AddressOf(points->data()) % alignof(Point) == 0);
    for (const Point& point : *points) {
        CHECK(point.x == 1.0F);
        CHECK(point.y == 2.0F);
    }

    REQUIRE(arena.Allocate(1, 1).has_value());
    const auto vectors = arena.AllocateArray<Vector4>(2);
    REQUIRE(vectors.has_value());
    CHECK(AddressOf(vectors->data()) % 16 == 0);
    CHECK((*vectors)[1].values[3] == 0.0F);
}

TEST_CASE("an array too large to count is refused") {
    FrameArena arena{1024};
    const auto refused = arena.AllocateArray<std::uint64_t>(std::numeric_limits<std::size_t>::max() / 4);
    REQUIRE_FALSE(refused.has_value());
    CHECK(arena.Used() == 0);
}

TEST_CASE("an alignment that is not a power of two is refused") {
    FrameArena arena{1024};
    for (const std::size_t alignment : {0U, 3U, 24U}) {
        CAPTURE(alignment);
        const auto refused = arena.Allocate(8, alignment);
        REQUIRE_FALSE(refused.has_value());
        CHECK(refused.error().message ==
              "the alignment must be a power of two, not " + std::to_string(alignment));
    }
    CHECK(arena.Used() == 0);
}

TEST_CASE("an empty piece needs no room, even in an arena with none") {
    FrameArena arena{0};
    const auto empty = arena.Allocate(0, 8);
    REQUIRE(empty.has_value());
    CHECK(empty->empty());
    CHECK_FALSE(arena.Allocate(1, 1).has_value());
    CHECK(arena.Capacity() == 0);
}

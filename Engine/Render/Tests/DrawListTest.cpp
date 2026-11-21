#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>

#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Render/DrawList.hpp"
#include "Nexum/Render/TextureStore.hpp"

using nexum::core::Rgba8;
using nexum::render::Blit;
using nexum::render::DrawList;
using nexum::render::TextureHandle;

namespace {

TextureHandle HandleNumber(std::uint32_t index) {
    return TextureHandle{.index = index, .generation = 1};
}

} // namespace

TEST_CASE("a new draw list has its clear color and no copies") {
    const DrawList black;
    CHECK(black.ClearColor() == Rgba8{.r = 0, .g = 0, .b = 0, .a = 255});
    CHECK(black.Blits().empty());

    const DrawList blue(Rgba8{.r = 0, .g = 0, .b = 255, .a = 255});
    CHECK(blue.ClearColor() == Rgba8{.r = 0, .g = 0, .b = 255, .a = 255});
}

TEST_CASE("[0000-CA-09] a draw list keeps its copies in order up to a fixed number and then refuses more") {
    DrawList list;
    for (std::size_t index = 0; index < DrawList::kMaxBlits; ++index) {
        const auto added = list.Add(Blit{.texture = HandleNumber(static_cast<std::uint32_t>(index)),
                                         .x = static_cast<std::int32_t>(index),
                                         .y = -static_cast<std::int32_t>(index)});
        REQUIRE(added.has_value());
    }
    const auto refused = list.Add(Blit{.texture = HandleNumber(999), .x = 0, .y = 0});
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message == "the draw list is full (64 copies)");

    REQUIRE(list.Blits().size() == DrawList::kMaxBlits);
    for (std::size_t index = 0; index < DrawList::kMaxBlits; ++index) {
        CAPTURE(index);
        CHECK(list.Blits()[index].texture == HandleNumber(static_cast<std::uint32_t>(index)));
        CHECK(list.Blits()[index].x == static_cast<std::int32_t>(index));
        CHECK(list.Blits()[index].y == -static_cast<std::int32_t>(index));
    }
}

TEST_CASE("a draw list starts over for a new frame") {
    DrawList list;
    REQUIRE(list.Add(Blit{.texture = HandleNumber(1), .x = 3, .y = 4}).has_value());
    list.Reset(Rgba8{.r = 1, .g = 2, .b = 3, .a = 4});
    CHECK(list.Blits().empty());
    CHECK(list.ClearColor() == Rgba8{.r = 1, .g = 2, .b = 3, .a = 4});
}

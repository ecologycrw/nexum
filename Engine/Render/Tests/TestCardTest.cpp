#include <doctest/doctest.h>

#include <array>
#include <cstdint>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Render/TestCard.hpp"

namespace {

using nexum::core::Rgba8;
using nexum::render::kTestCardBackground;
using nexum::render::MakeTestCard;

constexpr Rgba8 kWhite{.r = 255, .g = 255, .b = 255, .a = 255};
constexpr Rgba8 kBlack{.r = 0, .g = 0, .b = 0, .a = 255};

constexpr Rgba8 Gray(std::uint8_t level) {
    return Rgba8{.r = level, .g = level, .b = level, .a = 255};
}

} // namespace

TEST_CASE("the test card is 640 by 480 pixels, all of them opaque") {
    const nexum::core::Image card = MakeTestCard();
    REQUIRE(card.Width() == 640);
    REQUIRE(card.Height() == 480);
    for (const Rgba8& pixel : card.Pixels()) {
        REQUIRE(pixel.a == 255);
    }
}

TEST_CASE("the test card has 8 bars of 80 pixels in the order of the spec") {
    const nexum::core::Image card = MakeTestCard();
    constexpr std::array expected{
        kWhite,
        Rgba8{.r = 255, .g = 255, .b = 0, .a = 255},
        Rgba8{.r = 0, .g = 255, .b = 255, .a = 255},
        Rgba8{.r = 0, .g = 255, .b = 0, .a = 255},
        Rgba8{.r = 255, .g = 0, .b = 255, .a = 255},
        Rgba8{.r = 255, .g = 0, .b = 0, .a = 255},
        Rgba8{.r = 0, .g = 0, .b = 255, .a = 255},
        kBlack,
    };
    for (std::uint32_t bar = 0; bar < expected.size(); ++bar) {
        CAPTURE(bar);
        const std::uint32_t left = bar * 80;
        // The first columns of the white bar hold the corner mark; row 100 is below it.
        CHECK(card.At(left, 100) == expected[bar]);
        CHECK(card.At(left + 79, 100) == expected[bar]);
        CHECK(card.At(left + 40, 319) == expected[bar]);
    }
}

TEST_CASE("the gray scale of the test card goes from black to white, one level after the other") {
    const nexum::core::Image card = MakeTestCard();
    CHECK(card.At(0, 320) == Gray(0));
    CHECK(card.At(639, 399) == Gray(255));
    CHECK(card.At(320, 360) == Gray(128));
    std::uint8_t previous = 0;
    for (std::uint32_t x = 0; x < 640; ++x) {
        const Rgba8 top = card.At(x, 320);
        CAPTURE(x);
        REQUIRE(top == Gray(top.r));
        REQUIRE(card.At(x, 399) == top);
        REQUIRE(top.r >= previous);
        REQUIRE(top.r - previous <= 1);
        previous = top.r;
    }
}

TEST_CASE("the L of the test card stands upright in the top-left corner, with its foot to the right") {
    const nexum::core::Image card = MakeTestCard();
    // The black square behind the L.
    CHECK(card.At(0, 0) == kBlack);
    CHECK(card.At(47, 47) == kBlack);
    CHECK(card.At(48, 0) == kWhite);
    // The upright stroke, from the top of the L down to its foot.
    CHECK(card.At(8, 8) == kWhite);
    CHECK(card.At(15, 8) == kWhite);
    CHECK(card.At(16, 8) == kBlack);
    CHECK(card.At(8, 39) == kWhite);
    // The foot, at the bottom, going to the right only.
    CHECK(card.At(31, 39) == kWhite);
    CHECK(card.At(31, 32) == kWhite);
    CHECK(card.At(31, 31) == kBlack);
    CHECK(card.At(32, 39) == kBlack);
    CHECK(card.At(7, 39) == kBlack);
    CHECK(card.At(8, 40) == kBlack);
    // Nothing at the top right of the L.
    CHECK(card.At(28, 12) == kBlack);
}

TEST_CASE("the strip at the bottom of the test card has only the background color") {
    const nexum::core::Image card = MakeTestCard();
    for (std::uint32_t y = 400; y < 480; ++y) {
        for (std::uint32_t x = 0; x < 640; ++x) {
            REQUIRE(card.At(x, y) == kTestCardBackground);
        }
    }
}

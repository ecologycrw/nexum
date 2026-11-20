#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Rgba8.hpp"

using nexum::core::Image;
using nexum::core::Rgba8;

namespace {

constexpr Rgba8 kRed{.r = 255, .g = 0, .b = 0, .a = 255};
constexpr Rgba8 kBlue{.r = 0, .g = 0, .b = 255, .a = 255};

} // namespace

TEST_CASE("an image has its size and starts filled with one color") {
    const Image image{4, 3, kRed};
    CHECK(image.Width() == 4);
    CHECK(image.Height() == 3);
    CHECK(image.Pixels().size() == 12);
    CHECK(std::ranges::all_of(image.Pixels(), [](Rgba8 pixel) { return pixel == kRed; }));
}

TEST_CASE("the pixels are kept row by row, from the top-left corner") {
    Image image{4, 3, kRed};
    image.At(1, 0) = kBlue;
    image.At(0, 2) = kBlue;
    CHECK(image.Pixels()[1] == kBlue);
    CHECK(image.Pixels()[8] == kBlue);
    CHECK(image.At(3, 2) == kRed);
}

TEST_CASE("an image built from its pixels needs exactly width times height of them") {
    const auto built = Image::FromPixels(2, 2, std::vector<Rgba8>(4, kBlue));
    REQUIRE(built.has_value());
    CHECK(built->At(1, 1) == kBlue);

    const auto refused = Image::FromPixels(2, 2, std::vector<Rgba8>(3, kBlue));
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message == "an image of 2 x 2 needs 4 pixels, not 3");
}

TEST_CASE("two images are equal when their sizes and pixels are") {
    Image left{2, 2, kRed};
    const Image right{2, 2, kRed};
    CHECK(left == right);
    CHECK_FALSE(left == Image{4, 1, kRed});
    left.At(0, 0) = kBlue;
    CHECK_FALSE(left == right);
}

TEST_CASE("the default image is empty") {
    const Image image;
    CHECK(image.Width() == 0);
    CHECK(image.Height() == 0);
    CHECK(image.Pixels().empty());
}

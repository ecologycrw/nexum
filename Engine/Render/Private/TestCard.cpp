#include "Nexum/Render/TestCard.hpp"

#include <array>
#include <cstdint>

namespace nexum::render {

namespace {

constexpr core::Rgba8 kWhite{.r = 255, .g = 255, .b = 255, .a = 255};
constexpr core::Rgba8 kBlack{.r = 0, .g = 0, .b = 0, .a = 255};

constexpr std::array kBarColors{
    kWhite,
    core::Rgba8{.r = 255, .g = 255, .b = 0, .a = 255},
    core::Rgba8{.r = 0, .g = 255, .b = 255, .a = 255},
    core::Rgba8{.r = 0, .g = 255, .b = 0, .a = 255},
    core::Rgba8{.r = 255, .g = 0, .b = 255, .a = 255},
    core::Rgba8{.r = 255, .g = 0, .b = 0, .a = 255},
    core::Rgba8{.r = 0, .g = 0, .b = 255, .a = 255},
    kBlack,
};
constexpr std::uint32_t kBarWidth = kTestCardWidth / static_cast<std::uint32_t>(kBarColors.size());
static_assert(kBarWidth * kBarColors.size() == kTestCardWidth);

// The corner mark: a black square with a white L inside, 8 pixels from the top and the left of the square.
// The upright stroke is taller than the foot is long, so a turned L does not look like the right one.
constexpr std::uint32_t kMarkSquareSide = 48;
constexpr std::uint32_t kMarkMargin = 8;
constexpr std::uint32_t kMarkStroke = 8;
constexpr std::uint32_t kMarkHeight = 32;
constexpr std::uint32_t kMarkFootLength = 24;

struct Area {
    std::uint32_t left = 0;
    std::uint32_t top = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

void Fill(core::Image& image, Area area, core::Rgba8 color) noexcept {
    for (std::uint32_t y = area.top; y < area.top + area.height; ++y) {
        for (std::uint32_t x = area.left; x < area.left + area.width; ++x) {
            image.At(x, y) = color;
        }
    }
}

void DrawBars(core::Image& image) noexcept {
    for (std::uint32_t bar = 0; bar < kBarColors.size(); ++bar) {
        Fill(image,
             Area{.left = bar * kBarWidth, .top = 0, .width = kBarWidth, .height = kTestCardGrayScaleTop},
             kBarColors[bar]);
    }
}

// Column 0 is black and the last column is white; the columns between take the nearest gray level.
void DrawGrayScale(core::Image& image) noexcept {
    constexpr std::uint32_t kLastColumn = kTestCardWidth - 1;
    for (std::uint32_t x = 0; x < kTestCardWidth; ++x) {
        const auto level = static_cast<std::uint8_t>(((x * 255U) + (kLastColumn / 2U)) / kLastColumn);
        Fill(image,
             Area{.left = x,
                  .top = kTestCardGrayScaleTop,
                  .width = 1,
                  .height = kTestCardStripTop - kTestCardGrayScaleTop},
             core::Rgba8{.r = level, .g = level, .b = level, .a = 255});
    }
}

void DrawCornerMark(core::Image& image) noexcept {
    Fill(image, Area{.left = 0, .top = 0, .width = kMarkSquareSide, .height = kMarkSquareSide}, kBlack);
    Fill(image, Area{.left = kMarkMargin, .top = kMarkMargin, .width = kMarkStroke, .height = kMarkHeight},
         kWhite);
    Fill(image,
         Area{.left = kMarkMargin,
              .top = kMarkMargin + kMarkHeight - kMarkStroke,
              .width = kMarkFootLength,
              .height = kMarkStroke},
         kWhite);
}

} // namespace

core::Image MakeTestCard() {
    core::Image card(kTestCardWidth, kTestCardHeight, kTestCardBackground);
    DrawBars(card);
    DrawGrayScale(card);
    DrawCornerMark(card);
    return card;
}

} // namespace nexum::render

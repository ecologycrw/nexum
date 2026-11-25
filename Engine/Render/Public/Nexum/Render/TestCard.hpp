#pragma once

#include <cstdint>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Rgba8.hpp"

namespace nexum::render {

// The test card of the engine (specs/0000-fundacao/spec.md, "Prova visual"): plain colors that show at a
// glance whether the colors, the gray levels and the orientation of an image reach the screen unchanged. From
// the top:
// - 8 vertical bars of 80 pixels: white, yellow, cyan, green, magenta, red, blue and black;
// - a gray scale from black on the left to white on the right;
// - a strip of the background color, left free for something that moves over the card.
// A white "L" sits in the top-left corner, on a black square that keeps it visible over the white bar. Upside
// down or mirrored, the L shows it at once.
inline constexpr std::uint32_t kTestCardWidth = 640;
inline constexpr std::uint32_t kTestCardHeight = 480;

// The rows of each part, from the top of the card.
inline constexpr std::uint32_t kTestCardGrayScaleTop = 320;
inline constexpr std::uint32_t kTestCardStripTop = 400;
inline constexpr std::uint32_t kTestCardStripHeight = kTestCardHeight - kTestCardStripTop;

inline constexpr core::Rgba8 kTestCardBackground{.r = 32, .g = 36, .b = 48, .a = 255};

// The test card as an image, every pixel opaque.
[[nodiscard]] core::Image MakeTestCard();

} // namespace nexum::render

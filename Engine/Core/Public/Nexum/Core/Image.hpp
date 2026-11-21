#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "Nexum/Core/Assert.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Core/Rgba8.hpp"

namespace nexum::core {

// A picture in memory: width times height pixels of Rgba8, row by row from the top-left corner, the origin of
// the textures in the engine (docs/CONVENTIONS.md). The visual proof, the PNG files and the frames read back
// from the GPU all use it.
class Image {
public:
    Image() = default;

    // An image of `width` by `height` pixels, all of them `fill`.
    Image(std::uint32_t width, std::uint32_t height, Rgba8 fill = {});

    // An image made of pixels that came from elsewhere, such as a file or the GPU. Returns an error when
    // there are not exactly width times height of them (INV-12).
    [[nodiscard]] static Result<Image> FromPixels(std::uint32_t width, std::uint32_t height,
                                                  std::vector<Rgba8> pixels);

    [[nodiscard]] std::uint32_t Width() const noexcept { return m_width; }

    [[nodiscard]] std::uint32_t Height() const noexcept { return m_height; }

    [[nodiscard]] std::span<const Rgba8> Pixels() const noexcept { return m_pixels; }

    [[nodiscard]] std::span<Rgba8> Pixels() noexcept { return m_pixels; }

    // The pixel in column x and row y, counted from the top-left corner. Both must be inside the image.
    [[nodiscard]] Rgba8& At(std::uint32_t x, std::uint32_t y) noexcept {
        NEXUM_ASSERT(x < m_width && y < m_height);
        return m_pixels[IndexOf(x, y)];
    }

    [[nodiscard]] const Rgba8& At(std::uint32_t x, std::uint32_t y) const noexcept {
        NEXUM_ASSERT(x < m_width && y < m_height);
        return m_pixels[IndexOf(x, y)];
    }

    friend bool operator==(const Image&, const Image&) = default;

private:
    [[nodiscard]] std::size_t IndexOf(std::uint32_t x, std::uint32_t y) const noexcept {
        return (static_cast<std::size_t>(y) * m_width) + x;
    }

    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
    std::vector<Rgba8> m_pixels;
};

} // namespace nexum::core

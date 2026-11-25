#include "Nexum/Core/Image.hpp"

#include <string>
#include <utility>

namespace nexum::core {

namespace {

[[nodiscard]] std::size_t PixelCount(std::uint32_t width, std::uint32_t height) noexcept {
    return static_cast<std::size_t>(width) * height;
}

} // namespace

Image::Image(std::uint32_t width, std::uint32_t height, Rgba8 fill)
    : m_width(width), m_height(height), m_pixels(PixelCount(width, height), fill) {}

Result<Image> Image::FromPixels(std::uint32_t width, std::uint32_t height, std::vector<Rgba8> pixels) {
    const std::size_t expected = PixelCount(width, height);
    if (pixels.size() != expected) {
        return MakeError("an image of " + std::to_string(width) + " x " + std::to_string(height) + " needs " +
                         std::to_string(expected) + " pixels, not " + std::to_string(pixels.size()));
    }
    Image image;
    image.m_width = width;
    image.m_height = height;
    image.m_pixels = std::move(pixels);
    return image;
}

} // namespace nexum::core

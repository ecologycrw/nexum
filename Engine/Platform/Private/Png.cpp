#include "Nexum/Platform/Png.hpp"

#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

namespace nexum::platform {

namespace {

// The rows are copied byte by byte between the image and SDL, in the order red, green, blue and alpha.
static_assert(sizeof(core::Rgba8) == 4);
static_assert(std::is_trivially_copyable_v<core::Rgba8>);

struct DestroySurface {
    void operator()(SDL_Surface* surface) const noexcept { SDL_DestroySurface(surface); }
};

using Surface = std::unique_ptr<SDL_Surface, DestroySurface>;

// SDL takes paths in UTF-8.
[[nodiscard]] std::string ToUtf8(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    std::string converted;
    converted.reserve(text.size());
    for (const char8_t unit : text) {
        converted.push_back(static_cast<char>(unit));
    }
    return converted;
}

[[nodiscard]] bool FitsTheLimit(std::uint32_t width, std::uint32_t height) noexcept {
    return width > 0 && height > 0 && width <= kMaxPngSide && height <= kMaxPngSide;
}

[[nodiscard]] std::string SizeText(std::uint32_t width, std::uint32_t height) {
    return std::to_string(width) + "x" + std::to_string(height);
}

// Locks the pixels of a surface while they are read or written, when the surface needs it.
class SurfaceLock {
public:
    explicit SurfaceLock(SDL_Surface* surface) noexcept
        : m_surface(surface), m_locked(SDL_LockSurface(surface)) {}
    SurfaceLock(const SurfaceLock&) = delete;
    SurfaceLock& operator=(const SurfaceLock&) = delete;
    SurfaceLock(SurfaceLock&&) = delete;
    SurfaceLock& operator=(SurfaceLock&&) = delete;
    ~SurfaceLock() {
        if (m_locked) {
            SDL_UnlockSurface(m_surface);
        }
    }

    [[nodiscard]] bool Locked() const noexcept { return m_locked; }

private:
    SDL_Surface* m_surface;
    bool m_locked;
};

} // namespace

core::Result<core::Image> LoadPng(const std::filesystem::path& path) {
    const std::string file = ToUtf8(path);
    const Surface loaded{SDL_LoadPNG(file.c_str())};
    if (!loaded) {
        return core::MakeError("could not read the PNG file " + file + ": " + SDL_GetError());
    }
    const auto width = static_cast<std::uint32_t>(loaded->w > 0 ? loaded->w : 0);
    const auto height = static_cast<std::uint32_t>(loaded->h > 0 ? loaded->h : 0);
    if (!FitsTheLimit(width, height)) {
        return core::MakeError("the PNG file " + file + " is " + SizeText(width, height) +
                               " pixels; an image must be between 1 and " + std::to_string(kMaxPngSide) +
                               " pixels on each side");
    }

    const Surface rgba{SDL_ConvertSurface(loaded.get(), SDL_PIXELFORMAT_RGBA32)};
    if (!rgba) {
        return core::MakeError("could not convert the PNG file " + file + " to RGBA: " + SDL_GetError());
    }
    const SurfaceLock lock{rgba.get()};
    if (!lock.Locked()) {
        return core::MakeError("could not read the pixels of the PNG file " + file + ": " + SDL_GetError());
    }

    std::vector<core::Rgba8> pixels(static_cast<std::size_t>(width) * height);
    const std::span<core::Rgba8> destination(pixels);
    const auto* source = static_cast<const std::byte*>(rgba->pixels);
    const auto pitch = static_cast<std::size_t>(rgba->pitch);
    for (std::size_t row = 0; row < height; ++row) {
        const std::span<core::Rgba8> line = destination.subspan(row * width, width);
        std::memcpy(line.data(), source + (row * pitch), line.size_bytes());
    }
    return core::Image::FromPixels(width, height, std::move(pixels));
}

core::Result<void> SavePng(const core::Image& image, const std::filesystem::path& path) {
    const std::string file = ToUtf8(path);
    const std::uint32_t width = image.Width();
    const std::uint32_t height = image.Height();
    if (!FitsTheLimit(width, height)) {
        return core::MakeError("cannot write the PNG file " + file + ": the image is " +
                               SizeText(width, height) + " pixels, and it must be between 1 and " +
                               std::to_string(kMaxPngSide) + " pixels on each side");
    }

    const Surface surface{
        SDL_CreateSurface(static_cast<int>(width), static_cast<int>(height), SDL_PIXELFORMAT_RGBA32)};
    if (!surface) {
        return core::MakeError("could not prepare the PNG file " + file + ": " + SDL_GetError());
    }
    {
        const SurfaceLock lock{surface.get()};
        if (!lock.Locked()) {
            return core::MakeError("could not prepare the PNG file " + file + ": " + SDL_GetError());
        }
        const std::span<const core::Rgba8> source = image.Pixels();
        auto* destination = static_cast<std::byte*>(surface->pixels);
        const auto pitch = static_cast<std::size_t>(surface->pitch);
        for (std::size_t row = 0; row < height; ++row) {
            const std::span<const core::Rgba8> line = source.subspan(row * width, width);
            std::memcpy(destination + (row * pitch), line.data(), line.size_bytes());
        }
    }
    if (!SDL_SavePNG(surface.get(), file.c_str())) {
        return core::MakeError("could not write the PNG file " + file + ": " + SDL_GetError());
    }
    return {};
}

} // namespace nexum::platform

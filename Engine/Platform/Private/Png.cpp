#include "Nexum/Platform/Png.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
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

struct CloseStream {
    void operator()(SDL_IOStream* stream) const noexcept { static_cast<void>(SDL_CloseIO(stream)); }
};

using Stream = std::unique_ptr<SDL_IOStream, CloseStream>;

// The start of every PNG file, from the PNG specification (W3C, sections 5.2 and 11.2.2): the signature of 8
// bytes, then the first chunk, which is always IHDR: its length (13, big-endian), its type, and the width and
// the height, each a big-endian number of 4 bytes.
constexpr std::array<std::byte, 8> kSignature{std::byte{0x89}, std::byte{0x50}, std::byte{0x4E},
                                              std::byte{0x47}, std::byte{0x0D}, std::byte{0x0A},
                                              std::byte{0x1A}, std::byte{0x0A}};
constexpr std::array<std::byte, 8> kHeaderChunk{std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
                                                std::byte{0x0D}, std::byte{0x49}, std::byte{0x48},
                                                std::byte{0x44}, std::byte{0x52}};
constexpr std::size_t kWidthOffset = 16;
constexpr std::size_t kHeightOffset = 20;

[[nodiscard]] std::uint32_t ReadBigEndian(std::span<const std::byte> bytes, std::size_t offset) noexcept {
    std::uint32_t value = 0;
    for (const std::byte byte : bytes.subspan(offset, 4)) {
        value = (value << 8U) | std::to_integer<std::uint32_t>(byte);
    }
    return value;
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

// Reads the image only after its header says that its size fits the limit, so a small file that declares a
// huge image is refused before any memory is asked for its pixels.
[[nodiscard]] core::Result<core::Image> Decode(Stream stream, std::span<const std::byte> header,
                                               const std::string& file) {
    const auto size = ReadPngSize(header);
    if (!size) {
        return core::MakeError("could not read the PNG file " + file + ": " + size.error().message);
    }
    if (!FitsTheLimit(size->width, size->height)) {
        return core::MakeError("the PNG file " + file + " is " + SizeText(size->width, size->height) +
                               " pixels; an image must be between 1 and " + std::to_string(kMaxPngSide) +
                               " pixels on each side");
    }
    const Surface loaded{SDL_LoadPNG_IO(stream.release(), true)};
    if (!loaded) {
        return core::MakeError("could not read the PNG file " + file + ": " + SDL_GetError());
    }
    const auto width = static_cast<std::uint32_t>(loaded->w > 0 ? loaded->w : 0);
    const auto height = static_cast<std::uint32_t>(loaded->h > 0 ? loaded->h : 0);
    if (width != size->width || height != size->height) {
        return core::MakeError("the PNG file " + file + " declares " + SizeText(size->width, size->height) +
                               " pixels, but SDL read " + SizeText(width, height));
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

} // namespace

core::Result<PngSize> ReadPngSize(std::span<const std::byte> header) {
    if (header.size() < kPngHeaderSize) {
        return core::MakeError("it has " + std::to_string(header.size()) + " bytes, fewer than the " +
                               std::to_string(kPngHeaderSize) + " of a PNG header");
    }
    if (!std::equal(kSignature.begin(), kSignature.end(), header.begin())) {
        return core::MakeError("it does not start with the PNG signature");
    }
    if (!std::equal(kHeaderChunk.begin(), kHeaderChunk.end(), header.subspan(kSignature.size()).begin())) {
        return core::MakeError("its first chunk is not the 13 bytes of IHDR");
    }
    return PngSize{.width = ReadBigEndian(header, kWidthOffset),
                   .height = ReadBigEndian(header, kHeightOffset)};
}

core::Result<core::Image> LoadPng(const std::filesystem::path& path) {
    const std::string file = ToUtf8(path);
    Stream stream{SDL_IOFromFile(file.c_str(), "rb")};
    if (!stream) {
        return core::MakeError("could not read the PNG file " + file + ": " + SDL_GetError());
    }
    std::array<std::byte, kPngHeaderSize> header{};
    const std::size_t read = SDL_ReadIO(stream.get(), header.data(), header.size());
    if (SDL_SeekIO(stream.get(), 0, SDL_IO_SEEK_SET) != 0) {
        return core::MakeError("could not read the PNG file " + file + ": " + SDL_GetError());
    }
    return Decode(std::move(stream), std::span<const std::byte>(header).first(read), file);
}

core::Result<core::Image> DecodePng(std::span<const std::byte> bytes) {
    Stream stream{SDL_IOFromConstMem(bytes.data(), bytes.size())};
    if (!stream) {
        return core::MakeError(std::string("could not read the PNG file in memory: ") + SDL_GetError());
    }
    return Decode(std::move(stream), bytes.first(std::min(bytes.size(), kPngHeaderSize)), "in memory");
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

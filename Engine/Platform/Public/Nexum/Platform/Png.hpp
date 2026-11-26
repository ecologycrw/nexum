#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Result.hpp"

namespace nexum::platform {

// The largest side of a PNG image the engine reads or writes, in pixels.
inline constexpr std::uint32_t kMaxPngSide = 16384;

// The bytes at the start of a PNG file that hold its size: the signature and the IHDR chunk with the width
// and the height.
inline constexpr std::size_t kPngHeaderSize = 24;

struct PngSize {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// Reads the size that the header of a PNG declares, from its first kPngHeaderSize bytes. A header that is
// cut, that lacks the signature or whose first chunk is not IHDR is refused.
[[nodiscard]] core::Result<PngSize> ReadPngSize(std::span<const std::byte> header);

// Reads a PNG file into an RGBA image, whatever the format inside the file. SDL reads it (plan of slice 0,
// item 5): the approved images of the visual proof and the captures of the Nexum itself. Never a file of the
// original game (INV-08). The size comes from the header first, and an image wider or taller than kMaxPngSide
// is refused before SDL reads the pixels, so a small file cannot ask for a huge amount of memory.
[[nodiscard]] core::Result<core::Image> LoadPng(const std::filesystem::path& path);

// Reads a PNG held in memory, with the same checks as LoadPng.
[[nodiscard]] core::Result<core::Image> DecodePng(std::span<const std::byte> bytes);

// Writes an RGBA image as a PNG file, replacing the file if it exists. An empty image is refused.
[[nodiscard]] core::Result<void> SavePng(const core::Image& image, const std::filesystem::path& path);

} // namespace nexum::platform

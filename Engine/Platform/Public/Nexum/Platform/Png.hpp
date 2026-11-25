#pragma once

#include <cstdint>
#include <filesystem>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Result.hpp"

namespace nexum::platform {

// The largest side of a PNG image the engine reads or writes, in pixels.
inline constexpr std::uint32_t kMaxPngSide = 16384;

// Reads a PNG file into an RGBA image, whatever the format inside the file. SDL reads it (plan of slice 0,
// item 5), and its documentation says the reader is meant for trusted files: the approved images of the
// visual proof and the captures of the Nexum itself. Never a file of the original game (INV-08). An image
// wider or taller than kMaxPngSide is refused, but only after SDL has read the whole file, so the limit does
// not bound the memory of the reading (docs/TECH_DEBT.md, TD-0020).
[[nodiscard]] core::Result<core::Image> LoadPng(const std::filesystem::path& path);

// Writes an RGBA image as a PNG file, replacing the file if it exists. An empty image is refused.
[[nodiscard]] core::Result<void> SavePng(const core::Image& image, const std::filesystem::path& path);

} // namespace nexum::platform

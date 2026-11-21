#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Result.hpp"

namespace visualproof {

// The largest difference allowed in each channel of each pixel, in levels from 0 to 255. Each scene states
// its own tolerance next to it (docs/VISUAL.md): the one of slice 0 is 1 level (CA-07 of
// specs/0000-fundacao/spec.md).
struct Tolerance {
    std::uint8_t levels = 0;
};

// What an approved comparison measured.
struct Approval {
    std::uint64_t pixels = 0;
    // The largest difference in any channel of any pixel: never above the tolerance.
    std::uint8_t largestDifference = 0;
};

// Refuses an image without pixels or with a single color: an empty, black or single-color image fails
// whatever it is compared with (docs/VISUAL.md). `name` says which image it is, for the message.
[[nodiscard]] nexum::core::Result<void> CheckHasContent(const nexum::core::Image& image,
                                                        std::string_view name);

// Approves the capture when both images have content and the same size, and no channel of any pixel of the
// capture is farther than the tolerance from the same channel of the approved image. Otherwise the error says
// why: the sizes, or how many pixels are beyond the tolerance and the first of them, row by row.
[[nodiscard]] nexum::core::Result<Approval>
CompareImages(const nexum::core::Image& capture, const nexum::core::Image& approved, Tolerance tolerance);

// The same, with both images read from PNG files. A file that cannot be read is refused, with its path.
[[nodiscard]] nexum::core::Result<Approval> CompareFiles(const std::filesystem::path& capture,
                                                         const std::filesystem::path& approved,
                                                         Tolerance tolerance);

} // namespace visualproof

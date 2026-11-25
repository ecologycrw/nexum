#include "VisualProof/ImageComparison.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Platform/Png.hpp"

namespace visualproof {

namespace core = nexum::core;

namespace {

[[nodiscard]] std::string SizeText(const core::Image& image) {
    return std::to_string(image.Width()) + "x" + std::to_string(image.Height()) + " pixels";
}

[[nodiscard]] std::string LevelsText(std::uint8_t levels) {
    return std::to_string(levels) + (levels == 1 ? " level" : " levels");
}

[[nodiscard]] std::uint8_t ChannelDifference(std::uint8_t first, std::uint8_t second) noexcept {
    return static_cast<std::uint8_t>(std::abs(static_cast<int>(first) - static_cast<int>(second)));
}

// The largest difference among the four channels of two pixels.
[[nodiscard]] std::uint8_t PixelDifference(const core::Rgba8& first, const core::Rgba8& second) noexcept {
    return std::max({ChannelDifference(first.r, second.r), ChannelDifference(first.g, second.g),
                     ChannelDifference(first.b, second.b), ChannelDifference(first.a, second.a)});
}

// What the pixels beyond the tolerance were: how many, and where the first one is.
struct Beyond {
    std::uint64_t count = 0;
    std::size_t first = 0;
};

[[nodiscard]] core::Error BeyondError(const core::Image& capture, const core::Image& approved, Beyond beyond,
                                      Tolerance tolerance) {
    const auto x = static_cast<std::uint32_t>(beyond.first % capture.Width());
    const auto y = static_cast<std::uint32_t>(beyond.first / capture.Width());
    const std::string count = std::to_string(beyond.count) + " of " +
                              std::to_string(capture.Pixels().size()) +
                              (beyond.count == 1 ? " pixels is" : " pixels are");
    return core::Error{count + " beyond the tolerance of " + LevelsText(tolerance.levels) +
                       "; the first is at (" + std::to_string(x) + ", " + std::to_string(y) + "): " +
                       core::ToString(capture.At(x, y)) + " instead of " + core::ToString(approved.At(x, y))};
}

// Both images have the same size here.
[[nodiscard]] core::Result<Approval> ComparePixels(const core::Image& capture, const core::Image& approved,
                                                   Tolerance tolerance) {
    const auto captured = capture.Pixels();
    const auto expected = approved.Pixels();
    Approval approval{.pixels = captured.size()};
    Beyond beyond;
    for (std::size_t index = 0; index < captured.size(); ++index) {
        const std::uint8_t difference = PixelDifference(captured[index], expected[index]);
        if (difference <= tolerance.levels) {
            approval.largestDifference = std::max(approval.largestDifference, difference);
            continue;
        }
        if (beyond.count == 0) {
            beyond.first = index;
        }
        ++beyond.count;
    }
    if (beyond.count > 0) {
        return tl::unexpected(BeyondError(capture, approved, beyond, tolerance));
    }
    return approval;
}

[[nodiscard]] core::Result<core::Image> ReadImage(const std::filesystem::path& file, std::string_view name) {
    auto image = nexum::platform::LoadPng(file);
    if (!image) {
        return core::MakeError("the " + std::string(name) + " cannot be read: " + image.error().message);
    }
    return image;
}

} // namespace

core::Result<void> CheckHasContent(const core::Image& image, std::string_view name) {
    const auto pixels = image.Pixels();
    if (pixels.empty()) {
        return core::MakeError("the " + std::string(name) + " has no pixels");
    }
    const core::Rgba8 first = pixels.front();
    if (std::ranges::all_of(pixels, [&first](const core::Rgba8& pixel) { return pixel == first; })) {
        return core::MakeError("the " + std::string(name) + " has a single color, " + core::ToString(first) +
                               ": an empty, black or single-color image fails (docs/VISUAL.md)");
    }
    return {};
}

core::Result<Approval> CompareImages(const core::Image& capture, const core::Image& approved,
                                     Tolerance tolerance) {
    if (auto content = CheckHasContent(capture, "capture"); !content) {
        return tl::unexpected(content.error());
    }
    if (auto content = CheckHasContent(approved, "approved image"); !content) {
        return tl::unexpected(content.error());
    }
    if (capture.Width() != approved.Width() || capture.Height() != approved.Height()) {
        return core::MakeError("the capture is " + SizeText(capture) + ", but the approved image is " +
                               SizeText(approved));
    }
    return ComparePixels(capture, approved, tolerance);
}

core::Result<Approval> CompareFiles(const std::filesystem::path& capture,
                                    const std::filesystem::path& approved, Tolerance tolerance) {
    auto captured = ReadImage(capture, "capture");
    if (!captured) {
        return tl::unexpected(captured.error());
    }
    auto expected = ReadImage(approved, "approved image");
    if (!expected) {
        return tl::unexpected(expected.error());
    }
    return CompareImages(*captured, *expected, tolerance);
}

} // namespace visualproof

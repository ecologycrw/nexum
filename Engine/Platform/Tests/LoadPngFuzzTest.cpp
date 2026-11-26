// Fuzz target of the PNG reader (cmake/NexumFuzzing.cmake): any bytes give an image or an error, never a
// crash. The header is read first; only an image of up to a million pixels goes on to SDL, because a valid
// header can ask for up to 16384 x 16384 pixels, about 1 GB, more than the memory a fuzzer gives to one
// input. The refusal of a larger image by the header has tests of its own (PngTest.cpp).
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>

#include "Nexum/Platform/Png.hpp"

namespace {

constexpr std::uint64_t kMostPixels = 1024 * 1024;

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto bytes = std::as_bytes(std::span<const std::uint8_t>(data, size));
    const auto declared = nexum::platform::ReadPngSize(bytes);
    if (!declared || std::uint64_t{declared->width} * declared->height > kMostPixels) {
        return 0;
    }
    const auto image = nexum::platform::DecodePng(bytes);
    if (image && (image->Width() != declared->width || image->Height() != declared->height)) {
        std::abort();
    }
    return 0;
}

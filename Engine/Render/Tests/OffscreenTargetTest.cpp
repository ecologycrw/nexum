#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <utility>

#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Render/GpuDevice.hpp"
#include "Nexum/Render/OffscreenTarget.hpp"

using nexum::core::Rgba8;
using nexum::platform::VideoSystem;
using nexum::render::GpuDevice;
using nexum::render::GpuDeviceOptions;
using nexum::render::OffscreenTarget;
using nexum::render::TargetSize;

namespace nexum::core {

std::ostream& operator<<(std::ostream& stream, const Rgba8& pixel) {
    return stream << "(" << +pixel.r << ", " << +pixel.g << ", " << +pixel.b << ", " << +pixel.a << ")";
}

} // namespace nexum::core

namespace {

// Each test owns its video system and device, in the order the engine requires: video, device, target.
struct GpuFixture {
    VideoSystem video;
    GpuDevice device;
};

GpuFixture StartGpu() {
    auto video = VideoSystem::Start();
    REQUIRE_MESSAGE(video.has_value(), (video ? "" : video.error().message));
    auto device = GpuDevice::Create(*video, GpuDeviceOptions{});
    REQUIRE_MESSAGE(device.has_value(), (device ? "" : device.error().message));
    return GpuFixture{std::move(*video), std::move(*device)};
}

// Not square, so a swap of width and height shows up.
constexpr TargetSize kSize{.width = 64, .height = 48};

} // namespace

TEST_CASE("a cleared offscreen target reads back exactly the cleared color in every pixel") {
    GpuFixture gpu = StartGpu();
    auto target = OffscreenTarget::Create(gpu.device, kSize);
    REQUIRE_MESSAGE(target.has_value(), (target ? "" : target.error().message));

    // The second color proves that the read does not return the first clear again.
    for (const Rgba8 color : {Rgba8{12, 34, 56, 255}, Rgba8{255, 0, 128, 64}}) {
        CAPTURE(color);
        REQUIRE(target->Clear(color).has_value());
        const auto pixels = target->ReadPixels();
        REQUIRE_MESSAGE(pixels.has_value(), (pixels ? "" : pixels.error().message));
        REQUIRE(pixels->size() == std::size_t{kSize.width} * kSize.height);
        const auto different =
            std::ranges::find_if(*pixels, [&](const Rgba8& pixel) { return pixel != color; });
        CHECK_MESSAGE(different == pixels->end(), "first different pixel at index ",
                      different - pixels->begin());
    }
}

TEST_CASE("an offscreen target refuses an empty or oversized size") {
    GpuFixture gpu = StartGpu();
    for (const TargetSize size :
         {TargetSize{0, 10}, TargetSize{10, 0}, TargetSize{OffscreenTarget::kMaxSide + 1, 1}}) {
        CAPTURE(size.width);
        CAPTURE(size.height);
        const auto target = OffscreenTarget::Create(gpu.device, size);
        REQUIRE_FALSE(target.has_value());
        CHECK(target.error().message.starts_with("an offscreen target must be between 1 and 16384 pixels"));
    }
}

TEST_CASE("an offscreen target keeps its size") {
    GpuFixture gpu = StartGpu();
    const auto target = OffscreenTarget::Create(gpu.device, kSize);
    REQUIRE(target.has_value());
    CHECK(target->Size().width == kSize.width);
    CHECK(target->Size().height == kSize.height);
}

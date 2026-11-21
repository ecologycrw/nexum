#include <doctest/doctest.h>

#include <string>

#include "GpuFixture.hpp"
#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Platform/PlatformInfo.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Render/GpuBackend.hpp"
#include "Nexum/Render/GpuDevice.hpp"
#include "Nexum/Render/OffscreenTarget.hpp"

using nexum::core::Image;
using nexum::core::Rgba8;
using nexum::platform::CurrentOperatingSystem;
using nexum::platform::VideoSystem;
using nexum::render::GpuBackend;
using nexum::render::GpuDevice;
using nexum::render::GpuDeviceOptions;
using nexum::render::OffscreenTarget;
using nexum::render::PreferredBackends;
using nexum::render::TargetSize;
using nexum::render::ToString;
using nexum::render_tests::CheckSameImage;
using nexum::render_tests::GpuFixture;
using nexum::render_tests::PixelText;
using nexum::render_tests::StartGpu;
using nexum::render_tests::StartVideo;

namespace {

// Not square, so a swap of width and height shows up.
constexpr TargetSize kSize{.width = 64, .height = 48};

void ExpectClearReadsBack(OffscreenTarget& target, Rgba8 color) {
    const std::string clearColor = PixelText(color);
    CAPTURE(clearColor);
    const auto cleared = target.Clear(color);
    REQUIRE_MESSAGE(cleared.has_value(), (cleared ? "" : cleared.error().message));
    const auto image = target.ReadImage();
    REQUIRE_MESSAGE(image.has_value(), (image ? "" : image.error().message));
    CheckSameImage(*image, Image(kSize.width, kSize.height, color));
}

} // namespace

TEST_CASE("a cleared offscreen target reads back exactly the cleared color in every pixel") {
    GpuFixture gpu = StartGpu();
    auto target = OffscreenTarget::Create(gpu.device, kSize);
    REQUIRE_MESSAGE(target.has_value(), (target ? "" : target.error().message));

    // The second color proves that the read does not return the first clear again.
    ExpectClearReadsBack(*target, Rgba8{12, 34, 56, 255});
    ExpectClearReadsBack(*target, Rgba8{255, 0, 128, 64});
}

TEST_CASE("every backend this system offers clears and reads back exactly") {
    const VideoSystem video = StartVideo();
    int started = 0;
    for (const GpuBackend backend : PreferredBackends(CurrentOperatingSystem())) {
        CAPTURE(ToString(backend));
        auto device = GpuDevice::Create(video, GpuDeviceOptions{.forcedBackend = backend});
        if (!device) {
            // The CI machines have no Vulkan driver on Windows; the default order then falls to Direct3D 12.
            MESSAGE("not available here: ", device.error().message);
            continue;
        }
        ++started;
        MESSAGE("clearing with ", ToString(backend), " on ", device->Description().adapterName);
        auto target = OffscreenTarget::Create(*device, kSize);
        REQUIRE_MESSAGE(target.has_value(), (target ? "" : target.error().message));
        ExpectClearReadsBack(*target, Rgba8{200, 100, 50, 255});
    }
    CHECK(started >= 1);
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

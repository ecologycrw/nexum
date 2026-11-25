#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "GpuFixture.hpp"
#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Platform/PlatformInfo.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Render/DrawList.hpp"
#include "Nexum/Render/GpuBackend.hpp"
#include "Nexum/Render/GpuDevice.hpp"
#include "Nexum/Render/OffscreenTarget.hpp"
#include "Nexum/Render/TextureStore.hpp"

using nexum::core::Image;
using nexum::core::Rgba8;
using nexum::platform::CurrentOperatingSystem;
using nexum::platform::VideoSystem;
using nexum::render::Blit;
using nexum::render::DrawList;
using nexum::render::GpuBackend;
using nexum::render::GpuDevice;
using nexum::render::GpuDeviceOptions;
using nexum::render::OffscreenTarget;
using nexum::render::PreferredBackends;
using nexum::render::TargetSize;
using nexum::render::TextureHandle;
using nexum::render::TextureStore;
using nexum::render::ToString;
using nexum::render_tests::CheckSameImage;
using nexum::render_tests::GpuFixture;
using nexum::render_tests::StartGpu;
using nexum::render_tests::StartVideo;

namespace {

// Not square, so a swap of width and height shows up.
constexpr TargetSize kTargetSize{.width = 64, .height = 48};
constexpr Rgba8 kBackground{.r = 12, .g = 34, .b = 56, .a = 255};

// Every pixel different from the others and from the background, so a shift by one pixel, a swapped channel
// or a filtered pixel shows up. `tone` tells two patterns apart.
Image MakePattern(std::uint32_t width, std::uint32_t height, std::uint8_t tone) {
    Image image(width, height);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            image.At(x, y) = Rgba8{.r = static_cast<std::uint8_t>(100 + (x * 9)),
                                   .g = static_cast<std::uint8_t>(20 + (y * 20)),
                                   .b = tone,
                                   .a = static_cast<std::uint8_t>(255 - x)};
        }
    }
    return image;
}

// A copy as the test expects it: the image and where its top-left corner goes.
struct Placed {
    const Image* image = nullptr;
    std::int32_t x = 0;
    std::int32_t y = 0;
};

// What the target must hold, computed here on the CPU: the background, then each image pasted in order, with
// the parts outside the target left out.
Image ExpectedImage(std::span<const Placed> copies) {
    Image expected(kTargetSize.width, kTargetSize.height, kBackground);
    for (const Placed& copy : copies) {
        for (std::uint32_t y = 0; y < copy.image->Height(); ++y) {
            for (std::uint32_t x = 0; x < copy.image->Width(); ++x) {
                const std::int64_t targetX = std::int64_t{copy.x} + x;
                const std::int64_t targetY = std::int64_t{copy.y} + y;
                if (targetX >= 0 && targetY >= 0 && targetX < kTargetSize.width &&
                    targetY < kTargetSize.height) {
                    expected.At(static_cast<std::uint32_t>(targetX), static_cast<std::uint32_t>(targetY)) =
                        copy.image->At(x, y);
                }
            }
        }
    }
    return expected;
}

// The textures, the target and the list of one test, created in the order the engine requires.
struct CopyScene {
    TextureStore store;
    OffscreenTarget target;
};

CopyScene MakeScene(GpuDevice& device) {
    auto store = TextureStore::Create(device, 8);
    REQUIRE_MESSAGE(store.has_value(), (store ? "" : store.error().message));
    auto target = OffscreenTarget::Create(device, kTargetSize);
    REQUIRE_MESSAGE(target.has_value(), (target ? "" : target.error().message));
    return CopyScene{std::move(*store), std::move(*target)};
}

TextureHandle UploadOrFail(TextureStore& store, const Image& image) {
    auto texture = store.Upload(image);
    REQUIRE_MESSAGE(texture.has_value(), (texture ? "" : texture.error().message));
    return *texture;
}

// Draws the copies into the target and checks every pixel of the result against the CPU.
void DrawAndCheck(CopyScene& scene, std::span<const Placed> copies) {
    DrawList list(kBackground);
    for (const Placed& copy : copies) {
        const TextureHandle texture = UploadOrFail(scene.store, *copy.image);
        REQUIRE(list.Add(Blit{.texture = texture, .x = copy.x, .y = copy.y}).has_value());
    }
    const auto drawn = scene.target.Draw(list, scene.store);
    REQUIRE_MESSAGE(drawn.has_value(), (drawn ? "" : drawn.error().message));
    const auto image = scene.target.ReadImage();
    REQUIRE_MESSAGE(image.has_value(), (image ? "" : image.error().message));
    CheckSameImage(*image, ExpectedImage(copies));
}

} // namespace

TEST_CASE("[0000-CA-07] a texture copied into an offscreen target reads back exactly, pixel by pixel") {
    GpuFixture gpu = StartGpu();
    CopyScene scene = MakeScene(gpu.device);
    const Image pattern = MakePattern(8, 4, 200);
    const std::vector<Placed> copies{{.image = &pattern, .x = 10, .y = 20}};
    DrawAndCheck(scene, copies);
}

TEST_CASE("[0000-CA-07] the copies are cut at the edges of the target") {
    GpuFixture gpu = StartGpu();
    CopyScene scene = MakeScene(gpu.device);
    const Image pattern = MakePattern(8, 6, 150);
    const std::vector<Placed> copies{
        {.image = &pattern, .x = -3, .y = -2},  // the top-left corner is outside
        {.image = &pattern, .x = 60, .y = 45},  // the bottom-right corner is outside
        {.image = &pattern, .x = 100, .y = 10}, // all of it is outside
        {.image = &pattern, .x = 30, .y = -6},  // all of it is above, touching the edge
    };
    DrawAndCheck(scene, copies);
}

TEST_CASE("[0000-CA-07] a later copy covers the earlier ones where they overlap") {
    GpuFixture gpu = StartGpu();
    CopyScene scene = MakeScene(gpu.device);
    const Image first = MakePattern(10, 10, 40);
    const Image second = MakePattern(6, 5, 250);
    const std::vector<Placed> copies{{.image = &first, .x = 5, .y = 5}, {.image = &second, .x = 9, .y = 7}};
    DrawAndCheck(scene, copies);
}

TEST_CASE(
    "an offscreen target draws many more frames than it lets wait for the GPU, and keeps the last one") {
    GpuFixture gpu = StartGpu();
    CopyScene scene = MakeScene(gpu.device);
    const Image pattern = MakePattern(8, 4, 180);
    const TextureHandle texture = UploadOrFail(scene.store, pattern);

    // Each frame puts the copy in another place, with a clear every 10 frames. Every frame after the first
    // kFramesInFlight waits for the GPU to finish an older one.
    constexpr std::int32_t kFrames = 50;
    static_assert(kFrames > 10 * static_cast<std::int32_t>(OffscreenTarget::kFramesInFlight));
    DrawList list(kBackground);
    for (std::int32_t frame = 0; frame < kFrames; ++frame) {
        if (frame % 10 == 0) {
            const auto cleared = scene.target.Clear(Rgba8{.r = 255, .g = 0, .b = 0, .a = 255});
            REQUIRE_MESSAGE(cleared.has_value(), (cleared ? "" : cleared.error().message));
        }
        list.Reset(kBackground);
        REQUIRE(list.Add(Blit{.texture = texture, .x = frame, .y = frame % 40}).has_value());
        const auto drawn = scene.target.Draw(list, scene.store);
        REQUIRE_MESSAGE(drawn.has_value(), (drawn ? "" : drawn.error().message));
    }

    const auto image = scene.target.ReadImage();
    REQUIRE_MESSAGE(image.has_value(), (image ? "" : image.error().message));
    const std::vector<Placed> last{{.image = &pattern, .x = kFrames - 1, .y = (kFrames - 1) % 40}};
    CheckSameImage(*image, ExpectedImage(last));
}

TEST_CASE("[0000-CA-07] every backend this system offers copies textures exactly") {
    const VideoSystem video = StartVideo();
    int started = 0;
    for (const GpuBackend backend : PreferredBackends(CurrentOperatingSystem())) {
        CAPTURE(ToString(backend));
        auto device = GpuDevice::Create(video, GpuDeviceOptions{.forcedBackend = backend});
        if (!device) {
            MESSAGE("not available here: ", device.error().message);
            continue;
        }
        ++started;
        MESSAGE("copying with ", ToString(backend), " on ", device->Description().adapterName);
        CopyScene scene = MakeScene(*device);
        const Image first = MakePattern(12, 9, 90);
        const Image second = MakePattern(7, 7, 220);
        const std::vector<Placed> copies{{.image = &first, .x = -4, .y = 3},
                                         {.image = &second, .x = 58, .y = 40}};
        DrawAndCheck(scene, copies);
    }
    CHECK(started >= 1);
}

TEST_CASE("a frame that copies a released texture is refused and draws nothing") {
    GpuFixture gpu = StartGpu();
    CopyScene scene = MakeScene(gpu.device);
    const Rgba8 green{.r = 0, .g = 200, .b = 0, .a = 255};
    REQUIRE(scene.target.Clear(green).has_value());
    const TextureHandle texture = UploadOrFail(scene.store, MakePattern(4, 4, 10));
    REQUIRE(scene.store.Release(texture));

    DrawList list(kBackground);
    REQUIRE(list.Add(Blit{.texture = texture, .x = 0, .y = 0}).has_value());
    const auto refused = scene.target.Draw(list, scene.store);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message == "the draw list copies a texture that is not in the store");

    const auto image = scene.target.ReadImage();
    REQUIRE(image.has_value());
    CheckSameImage(*image, Image(kTargetSize.width, kTargetSize.height, green));
}

TEST_CASE("a frame with the textures of another device is refused") {
    GpuFixture gpu = StartGpu();
    CopyScene scene = MakeScene(gpu.device);
    auto otherDevice = GpuDevice::Create(gpu.video, GpuDeviceOptions{});
    REQUIRE_MESSAGE(otherDevice.has_value(), (otherDevice ? "" : otherDevice.error().message));
    auto otherStore = TextureStore::Create(*otherDevice, 1);
    REQUIRE(otherStore.has_value());
    const TextureHandle texture = UploadOrFail(*otherStore, MakePattern(4, 4, 10));

    DrawList list(kBackground);
    REQUIRE(list.Add(Blit{.texture = texture, .x = 0, .y = 0}).has_value());
    const auto refused = scene.target.Draw(list, *otherStore);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message == "the textures of the draw list belong to another GPU device");
}

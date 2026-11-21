#include <doctest/doctest.h>

#include <cstdint>
#include <utility>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Platform/Window.hpp"
#include "Nexum/Render/DrawList.hpp"
#include "Nexum/Render/GpuDevice.hpp"
#include "Nexum/Render/TextureStore.hpp"
#include "Nexum/Render/WindowTarget.hpp"

using nexum::core::Image;
using nexum::core::Rgba8;
using nexum::platform::VideoSystem;
using nexum::platform::Window;
using nexum::platform::WindowOptions;
using nexum::render::Blit;
using nexum::render::DrawList;
using nexum::render::FrameOutcome;
using nexum::render::GpuDevice;
using nexum::render::GpuDeviceOptions;
using nexum::render::TextureStore;
using nexum::render::WindowTarget;

TEST_CASE("a window connected to the GPU shows every cleared frame") {
    auto video = VideoSystem::Start();
    REQUIRE_MESSAGE(video.has_value(), (video ? "" : video.error().message));
    auto device = GpuDevice::Create(*video, GpuDeviceOptions{});
    REQUIRE_MESSAGE(device.has_value(), (device ? "" : device.error().message));
    const auto window =
        Window::Open(*video, WindowOptions{.title = "Nexum GPU window test", .width = 320, .height = 240});
    REQUIRE_MESSAGE(window.has_value(), (window ? "" : window.error().message));
    MESSAGE("video driver: ", video->DriverName());

    auto target = WindowTarget::Claim(*device, *window);
    REQUIRE_MESSAGE(target.has_value(), (target ? "" : target.error().message));

    constexpr int kFrames = 5;
    for (int frame = 0; frame < kFrames; ++frame) {
        CAPTURE(frame);
        const auto color = static_cast<std::uint8_t>(frame * 50);
        const auto outcome = target->PresentClear(Rgba8{color, 64, 128, 255});
        REQUIRE_MESSAGE(outcome.has_value(), (outcome ? "" : outcome.error().message));
        CHECK(*outcome == FrameOutcome::Shown);
    }
}

TEST_CASE("[0000-CA-10] a window shows frames that copy textures, and refuses a texture that is gone") {
    auto video = VideoSystem::Start();
    REQUIRE_MESSAGE(video.has_value(), (video ? "" : video.error().message));
    auto device = GpuDevice::Create(*video, GpuDeviceOptions{});
    REQUIRE_MESSAGE(device.has_value(), (device ? "" : device.error().message));
    const auto window =
        Window::Open(*video, WindowOptions{.title = "Nexum GPU copy test", .width = 320, .height = 240});
    REQUIRE_MESSAGE(window.has_value(), (window ? "" : window.error().message));
    auto store = TextureStore::Create(*device, 2);
    REQUIRE(store.has_value());
    auto target = WindowTarget::Claim(*device, *window);
    REQUIRE_MESSAGE(target.has_value(), (target ? "" : target.error().message));

    const auto texture = store->Upload(Image(32, 16, Rgba8{200, 50, 50, 255}));
    REQUIRE_MESSAGE(texture.has_value(), (texture ? "" : texture.error().message));
    DrawList list(Rgba8{0, 0, 64, 255});
    // The second copy is mostly outside the window: it is cut, not refused.
    REQUIRE(list.Add(Blit{.texture = *texture, .x = 10, .y = 10}).has_value());
    REQUIRE(list.Add(Blit{.texture = *texture, .x = 300, .y = 230}).has_value());

    constexpr int kFrames = 5;
    for (int frame = 0; frame < kFrames; ++frame) {
        CAPTURE(frame);
        const auto outcome = target->Present(list, *store);
        REQUIRE_MESSAGE(outcome.has_value(), (outcome ? "" : outcome.error().message));
        CHECK(*outcome == FrameOutcome::Shown);
    }

    REQUIRE(store->Release(*texture));
    const auto refused = target->Present(list, *store);
    REQUIRE_FALSE(refused.has_value());
    CHECK(refused.error().message == "the draw list copies a texture that is not in the store");
}

TEST_CASE("a window target can be moved and keeps presenting") {
    auto video = VideoSystem::Start();
    REQUIRE(video.has_value());
    auto device = GpuDevice::Create(*video, GpuDeviceOptions{});
    REQUIRE(device.has_value());
    const auto window =
        Window::Open(*video, WindowOptions{.title = "Nexum GPU window test", .width = 160, .height = 120});
    REQUIRE(window.has_value());
    auto claimed = WindowTarget::Claim(*device, *window);
    REQUIRE(claimed.has_value());

    WindowTarget target = std::move(*claimed);
    const auto outcome = target.PresentClear(Rgba8{0, 0, 0, 255});
    REQUIRE_MESSAGE(outcome.has_value(), (outcome ? "" : outcome.error().message));
    CHECK(*outcome == FrameOutcome::Shown);
}

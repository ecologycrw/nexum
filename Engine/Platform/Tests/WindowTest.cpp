#include <doctest/doctest.h>

#include <utility>

#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Platform/Window.hpp"

using nexum::platform::VideoSystem;
using nexum::platform::Window;
using nexum::platform::WindowId;
using nexum::platform::WindowOptions;

namespace {

VideoSystem StartVideo() {
    auto video = VideoSystem::Start();
    REQUIRE_MESSAGE(video.has_value(), (video ? "" : video.error().message));
    return std::move(*video);
}

} // namespace

TEST_CASE("a window opens with a drawing area of at least the asked size in pixels") {
    const VideoSystem video = StartVideo();
    const auto window =
        Window::Open(video, WindowOptions{.title = "Nexum window test", .width = 320, .height = 240});
    REQUIRE_MESSAGE(window.has_value(), (window ? "" : window.error().message));

    // A high density screen has more pixels than points, never fewer.
    const auto size = window->SizeInPixels();
    MESSAGE("window drawing area: ", size.width, "x", size.height, " pixels");
    CHECK(size.width >= 320);
    CHECK(size.height >= 240);
    CHECK(window->Native().handle != nullptr);
}

TEST_CASE("[0001-CA-13] a window that opens shows its drawing on the screen") {
    const VideoSystem video = StartVideo();
    const auto window =
        Window::Open(video, WindowOptions{.title = "Nexum visible", .width = 160, .height = 120});
    REQUIRE(window.has_value());
    // Minimizing has no automatic test: the Xvfb of the CI has no window manager (docs/modules/platform.md).
    CHECK(window->IsVisible());
}

TEST_CASE("each open window has an id of its own") {
    const VideoSystem video = StartVideo();
    const auto first =
        Window::Open(video, WindowOptions{.title = "Nexum first", .width = 160, .height = 120});
    const auto second =
        Window::Open(video, WindowOptions{.title = "Nexum second", .width = 160, .height = 120});
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());

    CHECK(first->Id() != WindowId{});
    CHECK(second->Id() != WindowId{});
    CHECK(first->Id() != second->Id());
}

TEST_CASE("a window refuses to be resized to an empty or oversized size") {
    const VideoSystem video = StartVideo();
    auto window = Window::Open(video, WindowOptions{.title = "Nexum", .width = 320, .height = 240});
    REQUIRE(window.has_value());
    for (const auto& [width, height] :
         {std::pair{0U, 240U}, std::pair{320U, 0U}, std::pair{320U, Window::kMaxSide + 1}}) {
        CAPTURE(width);
        CAPTURE(height);
        const auto resized = window->Resize(width, height);
        REQUIRE_FALSE(resized.has_value());
        CHECK(
            resized.error().message.starts_with("a window must be between 1 and 16384 points on each side"));
    }
}

TEST_CASE("a window refuses an empty or oversized size") {
    const VideoSystem video = StartVideo();
    for (const auto& [width, height] :
         {std::pair{0U, 240U}, std::pair{320U, 0U}, std::pair{Window::kMaxSide + 1, 240U}}) {
        CAPTURE(width);
        CAPTURE(height);
        const auto window =
            Window::Open(video, WindowOptions{.title = "Nexum", .width = width, .height = height});
        REQUIRE_FALSE(window.has_value());
        CHECK(window.error().message.starts_with("a window must be between 1 and 16384 points on each side"));
    }
}

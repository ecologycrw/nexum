#pragma once

// What the GPU tests share: the video system and the device in the order the engine requires, and the check
// of two images. It came from the third copy in the tests (docs/CONVENTIONS.md, "Regra de três").

#include <doctest/doctest.h>

#include <cstdint>
#include <optional>
#include <utility>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Render/GpuDevice.hpp"

namespace nexum::render_tests {

// Each test owns its video system and device: video first, then the device, then what the device makes.
struct GpuFixture {
    platform::VideoSystem video;
    render::GpuDevice device;
};

inline platform::VideoSystem StartVideo() {
    auto video = platform::VideoSystem::Start();
    REQUIRE_MESSAGE(video.has_value(), (video ? "" : video.error().message));
    return std::move(*video);
}

inline GpuFixture StartGpu() {
    platform::VideoSystem video = StartVideo();
    auto device = render::GpuDevice::Create(video, render::GpuDeviceOptions{});
    REQUIRE_MESSAGE(device.has_value(), (device ? "" : device.error().message));
    return GpuFixture{std::move(video), std::move(*device)};
}

// Checks two images pixel by pixel and names the first pixel that differs, with both values.
inline void CheckSameImage(const core::Image& actual, const core::Image& expected) {
    REQUIRE(actual.Width() == expected.Width());
    REQUIRE(actual.Height() == expected.Height());
    std::optional<std::pair<std::uint32_t, std::uint32_t>> different;
    for (std::uint32_t y = 0; y < actual.Height() && !different; ++y) {
        for (std::uint32_t x = 0; x < actual.Width() && !different; ++x) {
            if (actual.At(x, y) != expected.At(x, y)) {
                different = std::pair{x, y};
            }
        }
    }
    if (different) {
        const auto [x, y] = *different;
        FAIL_CHECK("first different pixel at (", x, ", ", y, "): ", core::ToString(actual.At(x, y)),
                   " instead of ", core::ToString(expected.At(x, y)));
    }
}

} // namespace nexum::render_tests

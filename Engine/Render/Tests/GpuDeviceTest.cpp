#include <doctest/doctest.h>

#include <algorithm>
#include <span>
#include <string>
#include <utility>

#include "GpuFixture.hpp"
#include "Nexum/Platform/PlatformInfo.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Render/GpuDevice.hpp"

using nexum::platform::CurrentOperatingSystem;
using nexum::platform::OperatingSystem;
using nexum::platform::VideoSystem;
using nexum::render::GpuBackend;
using nexum::render::GpuDevice;
using nexum::render::GpuDeviceOptions;
using nexum::render::PreferredBackends;
using nexum::render::ToString;
using nexum::render_tests::StartVideo;

namespace {

// A backend SDL GPU never offers on this system.
GpuBackend MissingBackend() {
    return CurrentOperatingSystem() == OperatingSystem::MacOs ? GpuBackend::Direct3D12 : GpuBackend::Metal;
}

} // namespace

TEST_CASE("[0000-CA-06] the GPU device starts with a backend of this system's order and names the card") {
    const VideoSystem video = StartVideo();
    const auto device = GpuDevice::Create(video, GpuDeviceOptions{});
    REQUIRE_MESSAGE(device.has_value(), (device ? "" : device.error().message));

    const auto& description = device->Description();
    MESSAGE("GPU: ", ToString(description.backend), " on ", description.adapterName, " (",
            description.driverName, " ", description.driverVersion, ")");
    const std::span<const GpuBackend> order = PreferredBackends(CurrentOperatingSystem());
    CHECK(std::ranges::find(order, description.backend) != order.end());
    CHECK_FALSE(description.adapterName.empty());
}

TEST_CASE("[0000-CA-06] a forced backend is the one that starts") {
    const VideoSystem video = StartVideo();
    const auto chosen = GpuDevice::Create(video, GpuDeviceOptions{});
    REQUIRE(chosen.has_value());
    const GpuBackend backend = chosen->Description().backend;

    const auto forced = GpuDevice::Create(video, GpuDeviceOptions{.forcedBackend = backend});
    REQUIRE_MESSAGE(forced.has_value(), (forced ? "" : forced.error().message));
    CHECK(forced->Description().backend == backend);
}

TEST_CASE("[0000-CA-06] forcing a backend this system lacks gives a clear error") {
    const VideoSystem video = StartVideo();
    const GpuBackend missing = MissingBackend();
    const auto device = GpuDevice::Create(video, GpuDeviceOptions{.forcedBackend = missing});
    REQUIRE_FALSE(device.has_value());
    CHECK(device.error().message == "no GPU driver could start (" + std::string(ToString(missing)) +
                                        ": this GPU driver does not exist on this system)");
}

TEST_CASE("a GPU device can be moved without losing its description") {
    const VideoSystem video = StartVideo();
    auto created = GpuDevice::Create(video, GpuDeviceOptions{});
    REQUIRE(created.has_value());
    const std::string adapter = created->Description().adapterName;
    const GpuDevice device = std::move(*created);
    CHECK(device.Description().adapterName == adapter);
}

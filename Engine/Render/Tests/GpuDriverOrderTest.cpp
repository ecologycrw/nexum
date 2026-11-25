#include <doctest/doctest.h>

#include <span>
#include <string>
#include <vector>

#include "Nexum/Render/GpuBackend.hpp"

using nexum::platform::OperatingSystem;
using nexum::render::GpuBackend;
using nexum::render::ParseGpuBackend;
using nexum::render::PreferredBackends;
using nexum::render::ToString;

namespace {

std::vector<GpuBackend> OrderOn(OperatingSystem system) {
    const std::span<const GpuBackend> order = PreferredBackends(system);
    return {order.begin(), order.end()};
}

} // namespace

TEST_CASE("[0000-CA-06] Windows tries Vulkan first and Direct3D 12 after it") {
    CHECK(OrderOn(OperatingSystem::Windows) == std::vector{GpuBackend::Vulkan, GpuBackend::Direct3D12});
}

TEST_CASE("[0000-CA-06] Linux uses Vulkan and macOS uses Metal") {
    CHECK(OrderOn(OperatingSystem::Linux) == std::vector{GpuBackend::Vulkan});
    CHECK(OrderOn(OperatingSystem::MacOs) == std::vector{GpuBackend::Metal});
}

TEST_CASE("the backend names are the ones typed on the command line") {
    for (const GpuBackend backend : {GpuBackend::Vulkan, GpuBackend::Direct3D12, GpuBackend::Metal}) {
        const auto parsed = ParseGpuBackend(ToString(backend));
        REQUIRE(parsed.has_value());
        CHECK(*parsed == backend);
    }
    CHECK(ToString(GpuBackend::Vulkan) == "vulkan");
    CHECK(ToString(GpuBackend::Direct3D12) == "direct3d12");
    CHECK(ToString(GpuBackend::Metal) == "metal");
}

TEST_CASE("[0000-CA-06] an unknown backend name gives a clear error") {
    for (const char* const name : {"opengl", "Vulkan", "", "vulkan "}) {
        CAPTURE(name);
        const auto parsed = ParseGpuBackend(name);
        REQUIRE_FALSE(parsed.has_value());
        CHECK(parsed.error().message ==
              "unknown GPU driver '" + std::string(name) + "': use vulkan, direct3d12 or metal");
    }
}

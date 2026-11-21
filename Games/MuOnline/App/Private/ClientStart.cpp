#include "ClientStart.hpp"

#include <string>

#include "Nexum/Render/GpuBackend.hpp"

namespace mu::app {

namespace core = nexum::core;
namespace platform = nexum::platform;
namespace render = nexum::render;

namespace {

constexpr nexum::debug::LogCategory kPlatform{"platform"};
constexpr nexum::debug::LogCategory kRender{"render"};

// "GPU backend vulkan on NVIDIA GeForce GTX 1060 6GB (driver NVIDIA 582.78)": the line CA-06 asks for.
[[nodiscard]] std::string Describe(const render::GpuDeviceDescription& gpu) {
    std::string line =
        "GPU backend " + std::string(render::ToString(gpu.backend)) + " on " +
        (gpu.adapterName.empty() ? std::string("a graphics card with no name") : gpu.adapterName);
    std::string driver = gpu.driverName;
    if (!gpu.driverVersion.empty()) {
        driver += (driver.empty() ? "" : " ") + gpu.driverVersion;
    }
    if (!driver.empty()) {
        line += " (driver " + driver + ")";
    }
    return line;
}

} // namespace

core::Result<platform::VideoSystem> StartVideo(nexum::debug::Logger& log) {
    auto video = platform::VideoSystem::Start();
    if (video) {
        log.Info(kPlatform, "video driver " + std::string(video->DriverName()));
    }
    return video;
}

core::Result<render::GpuDevice> CreateDevice(const platform::VideoSystem& video, const ClientOptions& options,
                                             nexum::debug::Logger& log) {
    auto device =
        render::GpuDevice::Create(video, render::GpuDeviceOptions{.forcedBackend = options.gpuBackend});
    if (device) {
        log.Info(kRender, Describe(device->Description()));
    }
    return device;
}

} // namespace mu::app

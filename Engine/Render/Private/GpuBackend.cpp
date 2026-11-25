#include "Nexum/Render/GpuBackend.hpp"

#include <array>
#include <string>

namespace nexum::render {

namespace {

constexpr std::array kWindowsOrder{GpuBackend::Vulkan, GpuBackend::Direct3D12};
constexpr std::array kLinuxOrder{GpuBackend::Vulkan};
constexpr std::array kMacOsOrder{GpuBackend::Metal};

constexpr std::array kAllBackends{GpuBackend::Vulkan, GpuBackend::Direct3D12, GpuBackend::Metal};

} // namespace

std::span<const GpuBackend> PreferredBackends(platform::OperatingSystem system) noexcept {
    switch (system) {
    case platform::OperatingSystem::Windows:
        return kWindowsOrder;
    case platform::OperatingSystem::Linux:
        return kLinuxOrder;
    case platform::OperatingSystem::MacOs:
        return kMacOsOrder;
    }
    return {};
}

std::string_view ToString(GpuBackend backend) noexcept {
    // The same names SDL GPU uses, so the value of --gpu-driver goes to SDL unchanged.
    switch (backend) {
    case GpuBackend::Vulkan:
        return "vulkan";
    case GpuBackend::Direct3D12:
        return "direct3d12";
    case GpuBackend::Metal:
        return "metal";
    }
    return "unknown";
}

core::Result<GpuBackend> ParseGpuBackend(std::string_view name) {
    for (const GpuBackend backend : kAllBackends) {
        if (name == ToString(backend)) {
            return backend;
        }
    }
    return core::MakeError("unknown GPU driver '" + std::string(name) + "': use vulkan, direct3d12 or metal");
}

} // namespace nexum::render

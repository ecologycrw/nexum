#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Platform/PlatformInfo.hpp"

namespace nexum::render {

// The graphics interfaces the engine can draw with, through SDL GPU (docs/adr/0029-sdl3-e-sdl-gpu.md).
enum class GpuBackend : std::uint8_t { Vulkan, Direct3D12, Metal };

// The order in which the backends are tried on each system (docs/adr/0029-sdl3-e-sdl-gpu.md).
// On Windows, Vulkan comes first while the Direct3D 12 sampler defect stays open (docs/TECH_DEBT.md,
// TD-0005).
[[nodiscard]] std::span<const GpuBackend> PreferredBackends(platform::OperatingSystem system) noexcept;

// The name used on the command line and in the log: "vulkan", "direct3d12" or "metal".
[[nodiscard]] std::string_view ToString(GpuBackend backend) noexcept;

// Reads a backend name typed by the user, such as the value of --gpu-driver (INV-12).
[[nodiscard]] core::Result<GpuBackend> ParseGpuBackend(std::string_view name);

} // namespace nexum::render

#pragma once

#include <memory>
#include <optional>
#include <string>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Platform/VideoSystem.hpp"
#include "Nexum/Render/GpuBackend.hpp"

namespace nexum::render {

struct GpuDeviceOptions {
    // Empty: the backends are tried in the order of PreferredBackends. Set: only this one (the --gpu-driver
    // option).
    std::optional<GpuBackend> forcedBackend;
};

// What the device runs on, for the log and the reports.
struct GpuDeviceDescription {
    GpuBackend backend = GpuBackend::Vulkan;
    // The graphics card, such as "NVIDIA GeForce RTX 4070", "llvmpipe (LLVM 17.0.6, 256 bits)" or
    // "Microsoft Basic Render Driver".
    std::string adapterName;
    std::string driverName;
    std::string driverVersion;
};

// The connection to the graphics card. Everything the GPU draws or stores belongs to one device,
// and is destroyed before it.
class GpuDevice {
public:
    // The video system must be running for the whole life of the device.
    [[nodiscard]] static core::Result<GpuDevice> Create(const platform::VideoSystem& video,
                                                        const GpuDeviceOptions& options);

    GpuDevice(GpuDevice&& other) noexcept;
    GpuDevice& operator=(GpuDevice&& other) noexcept;
    GpuDevice(const GpuDevice&) = delete;
    GpuDevice& operator=(const GpuDevice&) = delete;
    ~GpuDevice();

    [[nodiscard]] const GpuDeviceDescription& Description() const noexcept;

private:
    struct State;
    friend struct GpuDeviceAccess;

    explicit GpuDevice(std::unique_ptr<State> state) noexcept;

    std::unique_ptr<State> m_state;
};

} // namespace nexum::render

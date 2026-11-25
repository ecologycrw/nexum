#pragma once

#include <memory>

#include <SDL3/SDL.h>

#include "Nexum/Render/GpuDevice.hpp"

namespace nexum::render {

struct DestroyGpuDevice {
    void operator()(SDL_GPUDevice* device) const noexcept { SDL_DestroyGPUDevice(device); }
};

struct GpuDevice::State {
    std::unique_ptr<SDL_GPUDevice, DestroyGpuDevice> device;
    GpuDeviceDescription description;
};

// Gives the other parts of this module the SDL device, which the public header hides (INV-10).
struct GpuDeviceAccess {
    [[nodiscard]] static SDL_GPUDevice* Native(const GpuDevice& device) noexcept {
        return device.m_state->device.get();
    }
};

} // namespace nexum::render

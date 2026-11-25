#pragma once

#include <memory>

#include <SDL3/SDL.h>

namespace nexum::render {

// Owners of GPU objects, released through the device that created them. SDL frees each object only after the
// GPU has finished the commands that use it.
struct ReleaseGpuTexture {
    SDL_GPUDevice* device = nullptr;
    void operator()(SDL_GPUTexture* texture) const noexcept { SDL_ReleaseGPUTexture(device, texture); }
};

struct ReleaseGpuTransferBuffer {
    SDL_GPUDevice* device = nullptr;
    void operator()(SDL_GPUTransferBuffer* buffer) const noexcept {
        SDL_ReleaseGPUTransferBuffer(device, buffer);
    }
};

struct ReleaseGpuFence {
    SDL_GPUDevice* device = nullptr;
    void operator()(SDL_GPUFence* fence) const noexcept { SDL_ReleaseGPUFence(device, fence); }
};

using GpuTexturePointer = std::unique_ptr<SDL_GPUTexture, ReleaseGpuTexture>;
using GpuTransferBufferPointer = std::unique_ptr<SDL_GPUTransferBuffer, ReleaseGpuTransferBuffer>;
using GpuFencePointer = std::unique_ptr<SDL_GPUFence, ReleaseGpuFence>;

} // namespace nexum::render

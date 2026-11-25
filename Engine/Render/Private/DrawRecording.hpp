#pragma once

#include <SDL3/SDL.h>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Render/DrawList.hpp"
#include "Nexum/Render/TextureStore.hpp"

namespace nexum::render {

// Checks a frame before any GPU work starts: the store belongs to the device of the target, and every copy
// refers to a texture of the store.
[[nodiscard]] core::Result<void> CheckDrawList(SDL_GPUDevice* device, const DrawList& list,
                                               const TextureStore& store);

// Records a render pass that fills the whole target with one color.
[[nodiscard]] core::Result<void> RecordClear(SDL_GPUCommandBuffer* commands, SDL_GPUTexture* target,
                                             core::Rgba8 color);

// Records the clear and then the copies of the list, cut to a target of `targetSize` pixels. CheckDrawList
// must have accepted the list and the store. Nothing here asks for memory (INV-13).
[[nodiscard]] core::Result<void> RecordDrawList(SDL_GPUCommandBuffer* commands, SDL_GPUTexture* target,
                                                TextureSize targetSize, const DrawList& list,
                                                const TextureStore& store);

} // namespace nexum::render

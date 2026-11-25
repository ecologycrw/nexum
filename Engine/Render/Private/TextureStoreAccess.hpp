#pragma once

#include <cstdint>

#include <SDL3/SDL.h>

#include "GpuResources.hpp"
#include "Nexum/Core/HandlePool.hpp"
#include "Nexum/Render/TextureStore.hpp"

namespace nexum::render {

struct Texture {
    GpuTexturePointer gpu;
    TextureSize size;
};

struct TextureStore::State {
    SDL_GPUDevice* device = nullptr; // Not owned: the device outlives the store.
    core::HandlePool<Texture> textures;

    State(SDL_GPUDevice* owner, std::uint32_t capacity) : device(owner), textures(capacity) {}
};

// Gives the other parts of this module the SDL objects behind a store, which the public header hides
// (INV-10).
struct TextureStoreAccess {
    [[nodiscard]] static SDL_GPUDevice* Device(const TextureStore& store) noexcept {
        return store.m_state->device;
    }

    [[nodiscard]] static const Texture* Find(const TextureStore& store, TextureHandle texture) noexcept {
        return store.m_state->textures.Find(texture);
    }
};

} // namespace nexum::render

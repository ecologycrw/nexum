#include "DrawRecording.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>

#include "GpuColor.hpp"
#include "TextureStoreAccess.hpp"

namespace nexum::render {

namespace {

// The part of a copy that lands inside the target: where it starts in the texture and in the target, and its
// size. All of it in pixels.
struct CopyRegion {
    std::uint32_t sourceX = 0;
    std::uint32_t sourceY = 0;
    std::uint32_t targetX = 0;
    std::uint32_t targetY = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// Cuts a copy to the target. The sums use 64 bits, so a position near the limits of 32 bits cannot overflow.
[[nodiscard]] std::optional<CopyRegion> CutToTarget(const Blit& blit, TextureSize texture,
                                                    TextureSize target) noexcept {
    const std::int64_t x = blit.x;
    const std::int64_t y = blit.y;
    const std::int64_t left = std::max<std::int64_t>(x, 0);
    const std::int64_t top = std::max<std::int64_t>(y, 0);
    const std::int64_t right = std::min<std::int64_t>(x + texture.width, target.width);
    const std::int64_t bottom = std::min<std::int64_t>(y + texture.height, target.height);
    if (right <= left || bottom <= top) {
        return std::nullopt;
    }
    return CopyRegion{.sourceX = static_cast<std::uint32_t>(left - x),
                      .sourceY = static_cast<std::uint32_t>(top - y),
                      .targetX = static_cast<std::uint32_t>(left),
                      .targetY = static_cast<std::uint32_t>(top),
                      .width = static_cast<std::uint32_t>(right - left),
                      .height = static_cast<std::uint32_t>(bottom - top)};
}

[[nodiscard]] SDL_GPUBlitRegion Region(SDL_GPUTexture* texture, std::uint32_t x, std::uint32_t y,
                                       const CopyRegion& copy) noexcept {
    SDL_GPUBlitRegion region{};
    region.texture = texture;
    region.x = x;
    region.y = y;
    region.w = copy.width;
    region.h = copy.height;
    return region;
}

} // namespace

core::Result<void> CheckDrawList(SDL_GPUDevice* device, const DrawList& list, const TextureStore& store) {
    if (TextureStoreAccess::Device(store) != device) {
        return core::MakeError("the textures of the draw list belong to another GPU device");
    }
    for (const Blit& blit : list.Blits()) {
        if (TextureStoreAccess::Find(store, blit.texture) == nullptr) {
            return core::MakeError("the draw list copies a texture that is not in the store");
        }
    }
    return {};
}

core::Result<void> RecordClear(SDL_GPUCommandBuffer* commands, SDL_GPUTexture* target, core::Rgba8 color) {
    SDL_GPUColorTargetInfo info{};
    info.texture = target;
    info.clear_color = ToFloatColor(color);
    info.load_op = SDL_GPU_LOADOP_CLEAR;
    info.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* const pass = SDL_BeginGPURenderPass(commands, &info, 1, nullptr);
    if (pass == nullptr) {
        return tl::unexpected(SdlFailure("could not start the clear"));
    }
    SDL_EndGPURenderPass(pass);
    return {};
}

core::Result<void> RecordDrawList(SDL_GPUCommandBuffer* commands, SDL_GPUTexture* target,
                                  TextureSize targetSize, const DrawList& list, const TextureStore& store) {
    if (auto cleared = RecordClear(commands, target, list.ClearColor()); !cleared) {
        return cleared;
    }
    for (const Blit& blit : list.Blits()) {
        const Texture* const texture = TextureStoreAccess::Find(store, blit.texture);
        if (texture == nullptr) {
            return core::MakeError("the draw list copies a texture that is not in the store");
        }
        const std::optional<CopyRegion> copy = CutToTarget(blit, texture->size, targetSize);
        if (!copy) {
            continue;
        }
        // Same size on both sides and the nearest filter: each pixel of the texture lands on one pixel of the
        // target, unchanged.
        SDL_GPUBlitInfo info{};
        info.source = Region(texture->gpu.get(), copy->sourceX, copy->sourceY, *copy);
        info.destination = Region(target, copy->targetX, copy->targetY, *copy);
        info.load_op = SDL_GPU_LOADOP_LOAD;
        info.flip_mode = SDL_FLIP_NONE;
        info.filter = SDL_GPU_FILTER_NEAREST;
        info.cycle = false;
        SDL_BlitGPUTexture(commands, &info);
    }
    return {};
}

} // namespace nexum::render

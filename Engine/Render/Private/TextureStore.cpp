#include "Nexum/Render/TextureStore.hpp"

#include <cstring>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "GpuColor.hpp"
#include "GpuDeviceAccess.hpp"
#include "GpuResources.hpp"
#include "TextureStoreAccess.hpp"

namespace nexum::render {

namespace {

[[nodiscard]] core::Result<void> CheckImage(const core::Image& image) {
    const std::uint32_t width = image.Width();
    const std::uint32_t height = image.Height();
    if (width == 0 || height == 0 || width > TextureStore::kMaxSide || height > TextureStore::kMaxSide) {
        return core::MakeError("a texture must be between 1 and " + std::to_string(TextureStore::kMaxSide) +
                               " pixels on each side, not " + std::to_string(width) + "x" +
                               std::to_string(height));
    }
    return {};
}

[[nodiscard]] core::Result<GpuTexturePointer> CreateTexture(SDL_GPUDevice* device, std::uint32_t width,
                                                            std::uint32_t height) {
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    // A copy source must be a texture the GPU can sample.
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    GpuTexturePointer texture(SDL_CreateGPUTexture(device, &info), ReleaseGpuTexture{device});
    if (!texture) {
        return tl::unexpected(SdlFailure("could not create the texture"));
    }
    return texture;
}

// Puts the pixels in a buffer the GPU can read, row by row from the top, as core::Image keeps them.
[[nodiscard]] core::Result<GpuTransferBufferPointer> FillUploadBuffer(SDL_GPUDevice* device,
                                                                      const core::Image& image) {
    const auto pixels = image.Pixels();
    SDL_GPUTransferBufferCreateInfo info{};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    // At most 16384 x 16384 pixels of 4 bytes: 1 GiB, which fits in 32 bits.
    info.size = static_cast<std::uint32_t>(pixels.size_bytes());
    GpuTransferBufferPointer buffer(SDL_CreateGPUTransferBuffer(device, &info),
                                    ReleaseGpuTransferBuffer{device});
    if (!buffer) {
        return tl::unexpected(SdlFailure("could not create the upload buffer"));
    }
    void* const mapped = SDL_MapGPUTransferBuffer(device, buffer.get(), false);
    if (mapped == nullptr) {
        return tl::unexpected(SdlFailure("could not write the upload buffer"));
    }
    std::memcpy(mapped, pixels.data(), pixels.size_bytes());
    SDL_UnmapGPUTransferBuffer(device, buffer.get());
    return buffer;
}

// The copy runs on the GPU before any frame submitted later, so the texture is ready when a frame draws it.
[[nodiscard]] core::Result<void> SubmitUpload(SDL_GPUDevice* device, SDL_GPUTransferBuffer* buffer,
                                              SDL_GPUTexture* texture, std::uint32_t width,
                                              std::uint32_t height) {
    SDL_GPUCommandBuffer* const commands = SDL_AcquireGPUCommandBuffer(device);
    if (commands == nullptr) {
        return tl::unexpected(SdlFailure("could not start the GPU commands"));
    }
    SDL_GPUCopyPass* const copy = SDL_BeginGPUCopyPass(commands);
    if (copy == nullptr) {
        const core::Error error = SdlFailure("could not start the upload");
        SDL_CancelGPUCommandBuffer(commands);
        return tl::unexpected(error);
    }
    SDL_GPUTextureTransferInfo source{};
    source.transfer_buffer = buffer;
    source.pixels_per_row = width;
    source.rows_per_layer = height;
    SDL_GPUTextureRegion destination{};
    destination.texture = texture;
    destination.w = width;
    destination.h = height;
    destination.d = 1;
    SDL_UploadToGPUTexture(copy, &source, &destination, false);
    SDL_EndGPUCopyPass(copy);
    if (!SDL_SubmitGPUCommandBuffer(commands)) {
        return tl::unexpected(SdlFailure("could not submit the upload"));
    }
    return {};
}

} // namespace

core::Result<TextureStore> TextureStore::Create(GpuDevice& device, std::uint32_t capacity) {
    if (capacity == 0) {
        return core::MakeError("a texture store needs room for at least one texture");
    }
    return TextureStore(std::make_unique<State>(GpuDeviceAccess::Native(device), capacity));
}

TextureStore::TextureStore(std::unique_ptr<State> state) noexcept : m_state(std::move(state)) {}

TextureStore::TextureStore(TextureStore&& other) noexcept = default;

TextureStore& TextureStore::operator=(TextureStore&& other) noexcept = default;

TextureStore::~TextureStore() = default;

core::Result<TextureHandle> TextureStore::Upload(const core::Image& image) {
    if (auto valid = CheckImage(image); !valid) {
        return tl::unexpected(valid.error());
    }
    if (Count() == Capacity()) {
        return core::MakeError("the texture store is full (" + std::to_string(Capacity()) + " textures)");
    }
    SDL_GPUDevice* const device = m_state->device;
    auto texture = CreateTexture(device, image.Width(), image.Height());
    if (!texture) {
        return tl::unexpected(texture.error());
    }
    auto buffer = FillUploadBuffer(device, image);
    if (!buffer) {
        return tl::unexpected(buffer.error());
    }
    if (auto sent = SubmitUpload(device, buffer->get(), texture->get(), image.Width(), image.Height());
        !sent) {
        return tl::unexpected(sent.error());
    }
    const TextureSize size{.width = image.Width(), .height = image.Height()};
    return m_state->textures.Add(Texture{.gpu = std::move(*texture), .size = size});
}

bool TextureStore::Release(TextureHandle texture) noexcept {
    return m_state->textures.Remove(texture);
}

std::optional<TextureSize> TextureStore::SizeOf(TextureHandle texture) const noexcept {
    const Texture* const found = m_state->textures.Find(texture);
    if (found == nullptr) {
        return std::nullopt;
    }
    return found->size;
}

std::uint32_t TextureStore::Count() const noexcept {
    return m_state->textures.Size();
}

std::uint32_t TextureStore::Capacity() const noexcept {
    return m_state->textures.Capacity();
}

} // namespace nexum::render

#include "Nexum/Render/OffscreenTarget.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

#include "DrawRecording.hpp"
#include "GpuColor.hpp"
#include "GpuDeviceAccess.hpp"
#include "GpuResources.hpp"

namespace nexum::render {

namespace {

constexpr std::uint32_t kBytesPerPixel = 4;

[[nodiscard]] std::uint32_t ByteCount(TargetSize size) noexcept {
    return size.width * size.height * kBytesPerPixel;
}

[[nodiscard]] core::Result<void> CheckSize(TargetSize size) {
    if (size.width == 0 || size.height == 0 || size.width > OffscreenTarget::kMaxSide ||
        size.height > OffscreenTarget::kMaxSide) {
        return core::MakeError("an offscreen target must be between 1 and " +
                               std::to_string(OffscreenTarget::kMaxSide) + " pixels on each side, not " +
                               std::to_string(size.width) + "x" + std::to_string(size.height));
    }
    return {};
}

[[nodiscard]] core::Result<GpuTexturePointer> CreateColorTexture(SDL_GPUDevice* device, TargetSize size) {
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    info.width = size.width;
    info.height = size.height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    GpuTexturePointer texture(SDL_CreateGPUTexture(device, &info), ReleaseGpuTexture{device});
    if (!texture) {
        return tl::unexpected(SdlFailure("could not create the offscreen texture"));
    }
    return texture;
}

[[nodiscard]] core::Result<GpuTransferBufferPointer> CreateDownloadBuffer(SDL_GPUDevice* device,
                                                                          TargetSize size) {
    SDL_GPUTransferBufferCreateInfo info{};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = ByteCount(size);
    GpuTransferBufferPointer buffer(SDL_CreateGPUTransferBuffer(device, &info),
                                    ReleaseGpuTransferBuffer{device});
    if (!buffer) {
        return tl::unexpected(SdlFailure("could not create the download buffer"));
    }
    return buffer;
}

[[nodiscard]] std::vector<core::Rgba8> ToPixels(std::span<const std::uint8_t> bytes) {
    std::vector<core::Rgba8> pixels;
    pixels.reserve(bytes.size() / kBytesPerPixel);
    for (std::size_t index = 0; index + kBytesPerPixel <= bytes.size(); index += kBytesPerPixel) {
        pixels.push_back(core::Rgba8{bytes[index], bytes[index + 1], bytes[index + 2], bytes[index + 3]});
    }
    return pixels;
}

// Submits the commands and blocks until the GPU has run them.
[[nodiscard]] core::Result<void> SubmitAndWait(SDL_GPUDevice* device, SDL_GPUCommandBuffer* commands) {
    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    if (fence == nullptr) {
        return tl::unexpected(SdlFailure("could not submit the GPU commands"));
    }
    const bool finished = SDL_WaitForGPUFences(device, true, &fence, 1);
    SDL_ReleaseGPUFence(device, fence);
    if (!finished) {
        return tl::unexpected(SdlFailure("could not wait for the GPU"));
    }
    return {};
}

} // namespace

struct OffscreenTarget::State {
    SDL_GPUDevice* device = nullptr; // Not owned: the device outlives the target.
    TargetSize size;
    GpuTexturePointer texture;
    GpuTransferBufferPointer download;
};

core::Result<OffscreenTarget> OffscreenTarget::Create(GpuDevice& device, TargetSize size) {
    if (auto valid = CheckSize(size); !valid) {
        return tl::unexpected(valid.error());
    }
    SDL_GPUDevice* const native = GpuDeviceAccess::Native(device);
    auto texture = CreateColorTexture(native, size);
    if (!texture) {
        return tl::unexpected(texture.error());
    }
    auto download = CreateDownloadBuffer(native, size);
    if (!download) {
        return tl::unexpected(download.error());
    }
    auto state = std::make_unique<State>();
    state->device = native;
    state->size = size;
    state->texture = std::move(*texture);
    state->download = std::move(*download);
    return OffscreenTarget(std::move(state));
}

OffscreenTarget::OffscreenTarget(std::unique_ptr<State> state) noexcept : m_state(std::move(state)) {}

OffscreenTarget::OffscreenTarget(OffscreenTarget&& other) noexcept = default;

OffscreenTarget& OffscreenTarget::operator=(OffscreenTarget&& other) noexcept = default;

OffscreenTarget::~OffscreenTarget() = default;

TargetSize OffscreenTarget::Size() const noexcept {
    return m_state->size;
}

core::Result<void> OffscreenTarget::Clear(core::Rgba8 color) {
    SDL_GPUCommandBuffer* const commands = SDL_AcquireGPUCommandBuffer(m_state->device);
    if (commands == nullptr) {
        return tl::unexpected(SdlFailure("could not start the GPU commands"));
    }
    if (auto cleared = RecordClear(commands, m_state->texture.get(), color); !cleared) {
        SDL_CancelGPUCommandBuffer(commands);
        return cleared;
    }
    if (!SDL_SubmitGPUCommandBuffer(commands)) {
        return tl::unexpected(SdlFailure("could not submit the clear"));
    }
    return {};
}

core::Result<void> OffscreenTarget::Draw(const DrawList& list, const TextureStore& store) {
    if (auto valid = CheckDrawList(m_state->device, list, store); !valid) {
        return valid;
    }
    SDL_GPUCommandBuffer* const commands = SDL_AcquireGPUCommandBuffer(m_state->device);
    if (commands == nullptr) {
        return tl::unexpected(SdlFailure("could not start the GPU commands"));
    }
    const TextureSize size{.width = m_state->size.width, .height = m_state->size.height};
    if (auto drawn = RecordDrawList(commands, m_state->texture.get(), size, list, store); !drawn) {
        SDL_CancelGPUCommandBuffer(commands);
        return drawn;
    }
    if (!SDL_SubmitGPUCommandBuffer(commands)) {
        return tl::unexpected(SdlFailure("could not submit the frame"));
    }
    return {};
}

core::Result<core::Image> OffscreenTarget::ReadImage() {
    SDL_GPUCommandBuffer* const commands = SDL_AcquireGPUCommandBuffer(m_state->device);
    if (commands == nullptr) {
        return tl::unexpected(SdlFailure("could not start the GPU commands"));
    }
    SDL_GPUCopyPass* const copy = SDL_BeginGPUCopyPass(commands);
    if (copy == nullptr) {
        const core::Error error = SdlFailure("could not start the copy");
        SDL_CancelGPUCommandBuffer(commands);
        return tl::unexpected(error);
    }
    SDL_GPUTextureRegion source{};
    source.texture = m_state->texture.get();
    source.w = m_state->size.width;
    source.h = m_state->size.height;
    source.d = 1;
    SDL_GPUTextureTransferInfo destination{};
    destination.transfer_buffer = m_state->download.get();
    SDL_DownloadFromGPUTexture(copy, &source, &destination);
    SDL_EndGPUCopyPass(copy);
    if (auto done = SubmitAndWait(m_state->device, commands); !done) {
        return tl::unexpected(done.error());
    }

    const void* const mapped = SDL_MapGPUTransferBuffer(m_state->device, m_state->download.get(), false);
    if (mapped == nullptr) {
        return tl::unexpected(SdlFailure("could not read the download buffer"));
    }
    std::vector<core::Rgba8> pixels =
        ToPixels({static_cast<const std::uint8_t*>(mapped), ByteCount(m_state->size)});
    SDL_UnmapGPUTransferBuffer(m_state->device, m_state->download.get());
    return core::Image::FromPixels(m_state->size.width, m_state->size.height, std::move(pixels));
}

} // namespace nexum::render

#include "Nexum/Render/WindowTarget.hpp"

#include <utility>

#include <SDL3/SDL.h>

#include "GpuColor.hpp"
#include "GpuDeviceAccess.hpp"

namespace nexum::render {

struct WindowTarget::State {
    // Neither is owned: the device and the window outlive the target.
    SDL_GPUDevice* device = nullptr;
    SDL_Window* window = nullptr;

    State() = default;
    State(const State&) = delete;
    State& operator=(const State&) = delete;
    ~State() {
        if (device != nullptr && window != nullptr) {
            SDL_ReleaseWindowFromGPUDevice(device, window);
        }
    }
};

core::Result<WindowTarget> WindowTarget::Claim(GpuDevice& device, const platform::Window& window) {
    auto state = std::make_unique<State>();
    SDL_GPUDevice* const native = GpuDeviceAccess::Native(device);
    // The window handle is an SDL window: Engine/Platform and this module are the two owners of SDL (INV-10).
    SDL_Window* const sdlWindow = static_cast<SDL_Window*>(window.Native().handle);
    if (!SDL_ClaimWindowForGPUDevice(native, sdlWindow)) {
        return tl::unexpected(SdlFailure("could not connect the window to the GPU"));
    }
    state->device = native;
    state->window = sdlWindow;
    return WindowTarget(std::move(state));
}

WindowTarget::WindowTarget(std::unique_ptr<State> state) noexcept : m_state(std::move(state)) {}

WindowTarget::WindowTarget(WindowTarget&& other) noexcept = default;

WindowTarget& WindowTarget::operator=(WindowTarget&& other) noexcept = default;

WindowTarget::~WindowTarget() = default;

core::Result<FrameOutcome> WindowTarget::PresentClear(core::Rgba8 color) {
    SDL_GPUCommandBuffer* const commands = SDL_AcquireGPUCommandBuffer(m_state->device);
    if (commands == nullptr) {
        return tl::unexpected(SdlFailure("could not start the GPU commands"));
    }
    SDL_GPUTexture* screen = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, m_state->window, &screen, nullptr, nullptr)) {
        const core::Error error = SdlFailure("could not get the window's image");
        SDL_CancelGPUCommandBuffer(commands);
        return tl::unexpected(error);
    }
    FrameOutcome outcome = FrameOutcome::Skipped;
    if (screen != nullptr) {
        SDL_GPUColorTargetInfo target{};
        target.texture = screen;
        target.clear_color = ToFloatColor(color);
        target.load_op = SDL_GPU_LOADOP_CLEAR;
        target.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass* const pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
        if (pass == nullptr) {
            const core::Error error = SdlFailure("could not start the clear");
            SDL_CancelGPUCommandBuffer(commands);
            return tl::unexpected(error);
        }
        SDL_EndGPURenderPass(pass);
        outcome = FrameOutcome::Shown;
    }
    // Submitting also presents the image; with no image, it only releases the commands.
    if (!SDL_SubmitGPUCommandBuffer(commands)) {
        return tl::unexpected(SdlFailure("could not submit the frame"));
    }
    return outcome;
}

} // namespace nexum::render

#include "Nexum/Render/WindowTarget.hpp"

#include <utility>

#include <SDL3/SDL.h>

#include "DrawRecording.hpp"
#include "GpuColor.hpp"
#include "GpuDeviceAccess.hpp"

namespace nexum::render {

namespace {

// Gets the image of the window, lets `record` draw into it, and shows it. When the window has nothing to show
// into, as when it is minimized, nothing is drawn. The commands are submitted even when `record` fails,
// because SDL forbids cancelling commands that hold the image of a window. `record` is a template parameter,
// not a std::function, so a frame asks for no memory (INV-13).
template <typename Record>
[[nodiscard]] core::Result<FrameOutcome> PresentFrame(SDL_GPUDevice* device, SDL_Window* window,
                                                      Record record) {
    SDL_GPUCommandBuffer* const commands = SDL_AcquireGPUCommandBuffer(device);
    if (commands == nullptr) {
        return tl::unexpected(SdlFailure("could not start the GPU commands"));
    }
    SDL_GPUTexture* screen = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, window, &screen, &width, &height)) {
        const core::Error error = SdlFailure("could not get the window's image");
        SDL_CancelGPUCommandBuffer(commands);
        return tl::unexpected(error);
    }
    if (screen == nullptr) {
        // With no image, submitting only releases the commands.
        if (!SDL_SubmitGPUCommandBuffer(commands)) {
            return tl::unexpected(SdlFailure("could not submit the frame"));
        }
        return FrameOutcome::Skipped;
    }
    const core::Result<void> recorded =
        record(commands, screen, TextureSize{.width = width, .height = height});
    // Submitting also presents the image.
    const bool submitted = SDL_SubmitGPUCommandBuffer(commands);
    if (!recorded) {
        return tl::unexpected(recorded.error());
    }
    if (!submitted) {
        return tl::unexpected(SdlFailure("could not submit the frame"));
    }
    return FrameOutcome::Shown;
}

} // namespace

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
    return PresentFrame(m_state->device, m_state->window,
                        [color](SDL_GPUCommandBuffer* commands, SDL_GPUTexture* screen,
                                TextureSize /*size*/) { return RecordClear(commands, screen, color); });
}

core::Result<FrameOutcome> WindowTarget::Present(const DrawList& list, const TextureStore& store) {
    if (auto valid = CheckDrawList(m_state->device, list, store); !valid) {
        return tl::unexpected(valid.error());
    }
    return PresentFrame(
        m_state->device, m_state->window,
        [&list, &store](SDL_GPUCommandBuffer* commands, SDL_GPUTexture* screen, TextureSize size) {
            return RecordDrawList(commands, screen, size, list, store);
        });
}

} // namespace nexum::render

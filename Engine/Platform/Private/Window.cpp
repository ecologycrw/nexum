#include "Nexum/Platform/Window.hpp"

#include <limits>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

namespace nexum::platform {

namespace {

struct DestroyWindow {
    void operator()(SDL_Window* window) const noexcept { SDL_DestroyWindow(window); }
};

[[nodiscard]] core::Result<void> CheckSize(const WindowOptions& options) {
    if (options.width == 0 || options.height == 0 || options.width > Window::kMaxSide ||
        options.height > Window::kMaxSide) {
        return core::MakeError("a window must be between 1 and " + std::to_string(Window::kMaxSide) +
                               " points on each side, not " + std::to_string(options.width) + "x" +
                               std::to_string(options.height));
    }
    return {};
}

[[nodiscard]] std::uint32_t ToUnsigned(int value) noexcept {
    return value > 0 ? static_cast<std::uint32_t>(value) : 0;
}

} // namespace

struct Window::State {
    std::unique_ptr<SDL_Window, DestroyWindow> window;
};

// The video system is not used directly: asking for it proves that it is running.
core::Result<Window> Window::Open(const VideoSystem& /*video*/, const WindowOptions& options) {
    if (auto valid = CheckSize(options); !valid) {
        return tl::unexpected(valid.error());
    }
    static_assert(kMaxSide <= static_cast<std::uint32_t>(std::numeric_limits<int>::max()));
    auto state = std::make_unique<State>();
    state->window.reset(SDL_CreateWindow(options.title.c_str(), static_cast<int>(options.width),
                                         static_cast<int>(options.height), SDL_WINDOW_RESIZABLE));
    if (!state->window) {
        return core::MakeError(std::string("could not open the window: ") + SDL_GetError());
    }
    return Window(std::move(state));
}

Window::Window(std::unique_ptr<State> state) noexcept : m_state(std::move(state)) {}

Window::Window(Window&& other) noexcept = default;

Window& Window::operator=(Window&& other) noexcept = default;

Window::~Window() = default;

PixelSize Window::SizeInPixels() const {
    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(m_state->window.get(), &width, &height)) {
        return PixelSize{};
    }
    return PixelSize{.width = ToUnsigned(width), .height = ToUnsigned(height)};
}

NativeWindow Window::Native() const noexcept {
    return NativeWindow{.handle = m_state->window.get()};
}

} // namespace nexum::platform

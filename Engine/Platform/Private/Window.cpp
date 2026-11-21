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

[[nodiscard]] core::Result<void> CheckSize(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0 || width > Window::kMaxSide || height > Window::kMaxSide) {
        return core::MakeError("a window must be between 1 and " + std::to_string(Window::kMaxSide) +
                               " points on each side, not " + std::to_string(width) + "x" +
                               std::to_string(height));
    }
    return {};
}

[[nodiscard]] std::uint32_t ToUnsigned(int value) noexcept {
    return value > 0 ? static_cast<std::uint32_t>(value) : 0;
}

static_assert(Window::kMaxSide <= static_cast<std::uint32_t>(std::numeric_limits<int>::max()));

} // namespace

struct Window::State {
    std::unique_ptr<SDL_Window, DestroyWindow> window;
};

// The video system is not used directly: asking for it proves that it is running.
core::Result<Window> Window::Open(const VideoSystem& /*video*/, const WindowOptions& options) {
    if (auto valid = CheckSize(options.width, options.height); !valid) {
        return tl::unexpected(valid.error());
    }
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

WindowId Window::Id() const noexcept {
    return WindowId{.value = SDL_GetWindowID(m_state->window.get())};
}

core::Result<void> Window::Resize(std::uint32_t width, std::uint32_t height) {
    if (auto valid = CheckSize(width, height); !valid) {
        return valid;
    }
    if (!SDL_SetWindowSize(m_state->window.get(), static_cast<int>(width), static_cast<int>(height))) {
        return core::MakeError(std::string("could not resize the window: ") + SDL_GetError());
    }
    return {};
}

// The same event the system sends when the close button is pressed. SDL fills in the time of the event.
core::Result<void> Window::RequestClose() const {
    SDL_Event event{};
    event.window.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    event.window.windowID = SDL_GetWindowID(m_state->window.get());
    if (!SDL_PushEvent(&event)) {
        return core::MakeError(std::string("could not ask to close the window: ") + SDL_GetError());
    }
    return {};
}

NativeWindow Window::Native() const noexcept {
    return NativeWindow{.handle = m_state->window.get()};
}

} // namespace nexum::platform

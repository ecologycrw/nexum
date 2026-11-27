#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Platform/VideoSystem.hpp"

namespace nexum::platform {

struct WindowOptions {
    std::string title;
    // The size of the drawing area, in points. On a high density screen it holds more pixels.
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

struct PixelSize {
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    friend bool operator==(const PixelSize&, const PixelSize&) = default;
};

// Which window an event is about. The empty id belongs to no window.
struct WindowId {
    std::uint32_t value = 0;

    friend bool operator==(const WindowId&, const WindowId&) = default;
};

// The window as the operating system layer knows it. Only Engine/Render looks inside it (INV-10).
struct NativeWindow {
    void* handle = nullptr;
};

// A window on the screen. It must be destroyed after everything drawn in it and before its video system.
class Window {
public:
    // The largest side accepted, in points.
    static constexpr std::uint32_t kMaxSide = 16384;

    [[nodiscard]] static core::Result<Window> Open(const VideoSystem& video, const WindowOptions& options);

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    ~Window();

    // The size of the drawing area in pixels, which is what the GPU draws.
    [[nodiscard]] PixelSize SizeInPixels() const;

    [[nodiscard]] WindowId Id() const noexcept;

    // Whether the window shows its drawing on the screen: false while it is minimized, hidden or covered by
    // other windows, as the system last told SDL. Not every system says when a window is covered.
    [[nodiscard]] bool IsVisible() const noexcept;

    // Asks for a new size of the drawing area, in points, with the limits of Open. The window may take a few
    // frames to change; the program learns the new size from the WindowResized event
    // (Nexum/Platform/Application.hpp).
    [[nodiscard]] core::Result<void> Resize(std::uint32_t width, std::uint32_t height);

    // Asks to close the window, as its close button does: the program receives WindowCloseRequested between
    // two frames and decides what to do.
    [[nodiscard]] core::Result<void> RequestClose() const;

    [[nodiscard]] NativeWindow Native() const noexcept;

private:
    struct State;

    explicit Window(std::unique_ptr<State> state) noexcept;

    std::unique_ptr<State> m_state;
};

} // namespace nexum::platform

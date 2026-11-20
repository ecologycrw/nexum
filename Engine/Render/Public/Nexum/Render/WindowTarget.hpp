#pragma once

#include <cstdint>
#include <memory>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Platform/Window.hpp"
#include "Nexum/Render/GpuDevice.hpp"

namespace nexum::render {

// What happened to a frame sent to the window.
enum class FrameOutcome : std::uint8_t {
    Shown,
    // The window has nothing to show into, as when it is minimized. Nothing was drawn.
    Skipped,
};

// The screen of one window, connected to a GPU device.
// It must be destroyed before its device and before its window.
class WindowTarget {
public:
    [[nodiscard]] static core::Result<WindowTarget> Claim(GpuDevice& device, const platform::Window& window);

    WindowTarget(WindowTarget&& other) noexcept;
    WindowTarget& operator=(WindowTarget&& other) noexcept;
    WindowTarget(const WindowTarget&) = delete;
    WindowTarget& operator=(const WindowTarget&) = delete;
    ~WindowTarget();

    // Fills the window with one color and shows it.
    [[nodiscard]] core::Result<FrameOutcome> PresentClear(core::Rgba8 color);

private:
    struct State;

    explicit WindowTarget(std::unique_ptr<State> state) noexcept;

    std::unique_ptr<State> m_state;
};

} // namespace nexum::render

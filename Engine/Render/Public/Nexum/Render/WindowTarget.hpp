#pragma once

#include <cstdint>
#include <memory>

#include "Nexum/Core/Result.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Platform/Window.hpp"
#include "Nexum/Render/DrawList.hpp"
#include "Nexum/Render/GpuDevice.hpp"
#include "Nexum/Render/TextureStore.hpp"

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

    // Draws a frame and shows it: the clear color of the list, then its copies, in order, at the pixel size
    // the window has now. The handles of the list must come from `store`, a store of the same device: a
    // handle of another store of the same device is not caught, and may name another texture of `store`.
    // Nothing is drawn when the list or the store is refused. A frame never asks for memory (INV-13).
    [[nodiscard]] core::Result<FrameOutcome> Present(const DrawList& list, const TextureStore& store);

private:
    struct State;

    explicit WindowTarget(std::unique_ptr<State> state) noexcept;

    std::unique_ptr<State> m_state;
};

} // namespace nexum::render

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "Nexum/Core/Image.hpp"
#include "Nexum/Core/Result.hpp"
#include "Nexum/Core/Rgba8.hpp"
#include "Nexum/Render/DrawList.hpp"
#include "Nexum/Render/GpuDevice.hpp"
#include "Nexum/Render/TextureStore.hpp"

namespace nexum::render {

struct TargetSize {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// An image the GPU draws into without any window, and whose pixels can be read back.
// It is how the visual proof and the parity scenes capture a frame (docs/VISUAL.md).
// The target must be destroyed before its device.
class OffscreenTarget {
public:
    // The largest side Direct3D 12 guarantees for a texture.
    static constexpr std::uint32_t kMaxSide = 16384;

    // How many frames may wait for the GPU at once, as with a window. Drawing or clearing first waits for the
    // GPU to finish the oldest of them. Without the limit, a loop that draws faster than the GPU piles up
    // frames, and SDL asks for memory for each frame beyond the ones it keeps ready (INV-13).
    static constexpr std::size_t kFramesInFlight = 2;

    [[nodiscard]] static core::Result<OffscreenTarget> Create(GpuDevice& device, TargetSize size);

    OffscreenTarget(OffscreenTarget&& other) noexcept;
    OffscreenTarget& operator=(OffscreenTarget&& other) noexcept;
    OffscreenTarget(const OffscreenTarget&) = delete;
    OffscreenTarget& operator=(const OffscreenTarget&) = delete;
    ~OffscreenTarget();

    [[nodiscard]] TargetSize Size() const noexcept;

    // Fills the whole image with one color. It counts as a frame for kFramesInFlight.
    [[nodiscard]] core::Result<void> Clear(core::Rgba8 color);

    // Draws a frame: the clear color of the list, then its copies, in order. The textures must come from a
    // store of the same device. Nothing is drawn when the list or the store is refused. A frame never asks
    // for memory (INV-13), and it may wait for the GPU (kFramesInFlight).
    [[nodiscard]] core::Result<void> Draw(const DrawList& list, const TextureStore& store);

    // Waits for the GPU and copies the image back, with the pixels of everything drawn before.
    [[nodiscard]] core::Result<core::Image> ReadImage();

private:
    struct State;

    explicit OffscreenTarget(std::unique_ptr<State> state) noexcept;

    std::unique_ptr<State> m_state;
};

} // namespace nexum::render
